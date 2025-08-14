module Boxxy

  # Factory registers a new mouse-mode plugin called "Boxxy".
  class Factory < RBA::PluginFactory
    def initialize
      # Place near built-ins; choose an id larger than built-ins (>= 50000)
      # Use proper register overload: name (symbol), title, icon.
      register(50010, "boxxy:edit_mode", "Boxxy", ":box_24px.png")
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

    private

    # Pick the current layer or first drawing layer if none selected.
    # Sets @target_cv_idx and @target_layer_idx.
    def ensure_current_layer!
      iter = @view.current_layer
      if iter.is_null?
        it = @view.begin_layers
        while !it.at_end?
          n = it.current
          unless n.has_children?
            li = n.layer_index
            cv = n.cellview
            if cv >= 0 && li >= 0
              @view.current_layer = it
              iter = it
              break
            end
          end
          it.next
        end
      end

      if iter.is_null?
        raise "Please select or define a drawing layer first"
      end

      n = iter.current
      @target_cv_idx = n.cellview
      @target_layer_idx = n.layer_index
      if @target_cv_idx < 0 || @target_layer_idx < 0
        raise "Selected layer is not valid"
      end
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
      @marker = nil
    end

    def cancel_edit
      @editing = false
      @p1 = nil
      release_marker!
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
