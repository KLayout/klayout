module Boxxy

  # Helper: determine if KLayout runs in editor mode.
  def self.editor_mode?
    begin
      app = RBA::Application::instance
      # Prefer application-level flag if available
      if app && app.respond_to?(:is_editable?) && app.is_editable?
        return true
      end
      # Fallback: query current view
      mw = app && app.main_window
      view = mw && mw.current_view
      return !!(view && view.respond_to?(:is_editable?) && view.is_editable?)
    rescue
      false
    end
  end

  # Helper: configuration get/set for viewer warning opt-out
  def self.viewer_warning_suppressed?
    begin
      app = RBA::Application::instance
      v = app && app.get_config("boxxy_plugin.viewer_warning_suppressed")
      return v.to_s == "1"
    rescue
      false
    end
  end

  def self.suppress_viewer_warning!
    begin
      app = RBA::Application::instance
      app.set_config("boxxy_plugin.viewer_warning_suppressed", "1") if app
      app.commit_config if app && app.respond_to?(:commit_config)
    rescue
    end
  end

  # Factory registers a new mouse-mode plugin called "Boxxy".
  class Factory < RBA::PluginFactory
    def initialize
      # Only expose menus and register the tool in editor mode
      if Boxxy.editor_mode?
        # Create Edit > Boxxy submenu and actions (before register)
        add_submenu("boxxy_menu", "edit_menu.end", "Boxxy")
        add_menu_entry("boxxy_set_default", "boxxy_menu_set", "edit_menu.boxxy_menu.end", "Set Boxxy Default Layer From Current")
        add_menu_entry("boxxy_clear_default", "boxxy_menu_clear", "edit_menu.boxxy_menu.end", "Clear Boxxy Default Layer")

        register(50010, "boxxy:edit_mode", "Boxxy", ":box_24px.png")
      else
        # In viewer mode: do not register; optionally show warning once
        unless Boxxy.viewer_warning_suppressed?
          mw = RBA::Application::instance && RBA::Application::instance.main_window
          text = "Boxxy is available in Editor mode only.\n\n" \
                 "Start KLayout in Editor mode (e.g. with -e) to use this plugin."
          mb = RBA::QMessageBox::new(RBA::QMessageBox::Warning, "Boxxy", text, RBA::QMessageBox::Ok, mw)
          cb = RBA::QCheckBox::new("Don't show this again")
          mb.setCheckBox(cb)
          mb.exec
          Boxxy.suppress_viewer_warning! if cb.isChecked
        end
      end
    end
  
    def create_plugin(manager, dispatcher, view)
      Boxxy::Tool.new(view)
    end
  end

  # Tool implements interactive box creation.
  class Tool < RBA::Plugin
    def initialize(view)
      super()
      @view = view
      @editing = false
      @p1 = nil
      @marker = nil
    end

    # Activated when user selects the tool.
    def activated
      cancel_edit
    end

    def deactivated
      cancel_edit
    end

    # Start or finish the box on click.
    def mouse_click_event(p, buttons, prio)
      return false unless prio

      if !@editing
        # Ensure we have a current layer; auto-select the first if none.
        ensure_current_layer!
        @p1 = p
        @editing = true
        ensure_marker!
        update_marker(p)
        set_cursor(RBA::Cursor::Cross)
        true
      else
        # Finish: insert the box and stop editing.
        insert_box(@p1, p)
        cancel_edit
        true
      end
    rescue => e
      # Forward the error to KLayout status; don't crash the event loop.
      RBA::MessageBox::warning("Boxxy", e.message, RBA::MessageBox::b_ok)
      cancel_edit
      true
    end

    # Update preview while moving.
    def mouse_moved_event(p, buttons, prio)
      return false unless prio
      if @editing
        ensure_marker!
        update_marker(p)
        set_cursor(RBA::Cursor::Cross)
        return true
      end
      false
    end

    # ESC cancels the edit.
    def key_event(key, buttons)
      # Qt::Key_Escape == 0x01000000
      if @editing && key == 0x01000000
        cancel_edit
        return true
      end
      false
    end

    # Handle Boxxy menu actions
    def menu_activated(symbol)
      case symbol
      when "boxxy_set_default"
        iter = @view.current_layer
        if iter.is_null?
          RBA::MessageBox::warning("Boxxy", "No current layer is selected.", RBA::MessageBox::Ok)
          return true
        end
        n = iter.current
        cv = n.cellview
        li = n.layer_index
        if cv < 0 || li < 0
          RBA::MessageBox::warning("Boxxy", "Current layer is not a valid drawing layer.", RBA::MessageBox::Ok)
          return true
        end
        Boxxy.set_default_layer(cv, li)
        RBA::MessageBox::info("Boxxy", "Default layer set to CV=#{cv}, layer=#{li}.", RBA::MessageBox::Ok)
        true
      when "boxxy_clear_default"
        Boxxy.clear_default_layer
        RBA::MessageBox::info("Boxxy", "Default layer cleared.", RBA::MessageBox::Ok)
        true
      else
        false
      end
    end

    private

    # Determine target layer: prefer explicit selection; else default; else first drawing layer
    # Sets @target_cv_idx and @target_layer_idx.

    def ensure_current_layer!
      # Build chosen layer iterator explicitly:
      chosen = nil

      # If there is an explicit selection, use the current layer (native behavior)
      begin
        sel = @view.selected_layers
      rescue
        sel = []
      end
      sel = [] if sel.nil?
      if !sel.empty?
        chosen = @view.current_layer
      end

      # If no selection, try Boxxy's default layer first
      if chosen.nil? && Boxxy.default_layer?
        cv, li = Boxxy.get_default_layer
        if cv && li && cv >= 0 && li >= 0
          it = @view.begin_layers
          while !it.at_end?
            n = it.current
            unless n.has_children?
              if n.cellview == cv && n.layer_index == li
                chosen = it
                break
              end
            end
            it.next
          end
        end
      end

      # If still none, fall back to first drawing layer
      if chosen.nil?
        it = @view.begin_layers
        while !it.at_end?
          n = it.current
          unless n.has_children?
            li = n.layer_index
            cv = n.cellview
            if cv >= 0 && li >= 0
              chosen = it
              break
            end
          end
          it.next
        end
      end

      raise "Please select or define a drawing layer first" if chosen.nil?

      # Apply chosen iterator and cache indices
      @view.current_layer = chosen
      n = chosen.current
      @target_cv_idx = n.cellview
      @target_layer_idx = n.layer_index
      raise "Selected layer is not valid" if @target_cv_idx < 0 || @target_layer_idx < 0
    end

    def ensure_marker!
      if @marker.nil?
        @marker = RBA::Marker::new(@view)
        @marker.dismissable = true
        @marker.line_width = 1
        @marker.vertex_size = 0
        @marker.dither_pattern = -1
      end
    end

    def release_marker!
      if @marker
        begin
          if @marker.respond_to?(:destroy)
            @marker.destroy
          elsif @marker.respond_to?(:_destroy)
            @marker._destroy
          end
        rescue
          # ignore
        end
      end
      @marker = nil
    end

    def cancel_edit
      @editing = false
      @p1 = nil
      release_marker!
      begin
        set_cursor(RBA::Cursor::None_)
      rescue
      end
    end

    def update_marker(p2)
      return unless @marker && @p1
      box = RBA::DBox::new(@p1, p2)
      @marker.set(box)
    end

    # Insert a box into the target layer, mapping view coords to cell coords.
    def insert_box(p1, p2)
      raise "No target layer" unless @target_cv_idx && @target_layer_idx

      cv = @view.cellview(@target_cv_idx)
      raise "No active cellview" unless cv && cv.is_valid?

      layout = cv.layout
      cell = cv.cell
      raise "No target cell" unless cell

      # Map from view DPoint (micron) into the target cell's DPoint using inverse context transform.
      dct_inv = cv.context_dtrans.inverted
      q1 = dct_inv.trans(p1)
      q2 = dct_inv.trans(p2)
      dbox = RBA::DBox::new(q1, q2)

      # Convert to integer dbu space and insert
      ibox = dbox.to_itype(layout.dbu)
      cell.shapes(@target_layer_idx).insert(ibox)
    end
  end

end

# Register the factory (instantiation triggers registration)
Boxxy::Factory::new

# Module-level storage for default layer (session-scoped)
module Boxxy
  @@default_cv = nil
  @@default_li = nil

  def self.set_default_layer(cv, li)
    @@default_cv = cv
    @@default_li = li
  end

  def self.clear_default_layer
    @@default_cv = nil
    @@default_li = nil
  end

  def self.get_default_layer
    return @@default_cv, @@default_li
  end

  def self.default_layer?
    !@@default_cv.nil? && !@@default_li.nil?
  end
end
