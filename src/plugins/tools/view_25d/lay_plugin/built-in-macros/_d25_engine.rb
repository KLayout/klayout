# $autorun-early
# $priority: 1

require 'pathname'

module D25

  class D25ZInfo

    attr_accessor :layer, :zstart, :zstop, :display

    def initialize(_layer, _zstart, _zstop, _display)
      self.layer = _layer
      self.zstart = _zstart
      self.zstop = _zstop
      self.display = _display
    end

  end

  class D25Display

    attr_accessor :fill, :frame, :like, :name

    def initialize
      self.fill = nil
      self.frame = nil
      self.like = nil
      self.name = nil
    end

    def set_fill(arg)
      if !arg.is_a?(0xffffff.class)
        raise("'fill' must be a color value (an integer)")
      end
      self.fill = arg
    end

    def set_frame(arg)
      if !arg.is_a?(0xffffff.class)
        raise("'frame' must be a color value (an integer)")
      end
      self.frame = arg
    end

    def set_color(arg)
      if !arg.is_a?(0xffffff.class)
        raise("'color' must be a color value (an integer)")
      end
      self.fill = arg
      self.frame = nil
    end

    def set_like(arg)
      li = nil
      if arg.is_a?(String)
        li = RBA::LayerInfo::from_string(arg)
      elsif arg.is_a?(RBA::LayerInfo)
        li = arg
      else
        raise("'like' must be a string or LayerInfo object")
      end
      self.like = li
    end

  end

  # The D25 engine
  
  class D25Engine < DRC::DRCEngine
  
    def initialize

      super

      @current_z = 0.0
      @zstack = []

      # clip to layout view
      if ! RBA::LayoutView::current
        raise "No layout loaded for running 2.5d view on"
      end 

      self.region_overlap(RBA::LayoutView::current.box)

    end

    def z(*args)

      self._context("z") do

        layer = nil
        zstart = nil
        zstop = nil
        height = nil
        display = D25Display::new

        args.each do |a|

          if a.is_a?(Range)

            zstart = a.min
            zstop = a.max

          elsif a.is_a?(DRC::DRCLayer)

            if layer
              raise("Duplicate layer argument")
            end
            layer = a

          elsif a.is_a?(1.class) || a.is_a?(1.0.class)

            if height
              raise("Duplicate height specification")
            end
            height = a

          elsif a.is_a?(Hash)

            if a[:height]
              if height
                raise("Duplicate height specification")
              end
              height = a[:height]
            end

            if a[:zstart]
              if zstart
                raise("Duplicate zstart specification")
              end
              zstart = a[:zstart]
            end

            if a[:zstop]
              if zstop
                raise("Duplicate zstop specification")
              end
              zstop = a[:zstop]
            end  

            a[:color] && display.set_color(a[:color])
            a[:frame] && display.set_frame(a[:frame])
            a[:fill] && display.set_fill(a[:fill])
            a[:like] && display.set_like(a[:like])

            if a[:name]
              display.name = a[:name].to_s
            end

            invalid_keys = a.keys.select { |k| ![ :height, :zstart, :zstop, :color, :frame, :fill, :like, :name ].member?(k) }
            if invalid_keys.size > 0
              raise("Keyword argument(s) not understood: #{invalid_keys.collect(&:to_s).join(',')}")
            end

          else
            raise("Argument not understood: #{a.inspect}")
          end

        end

        if ! zstart
          zstart = @current_z
        end
        if ! zstop && ! height
          raise("Either height or zstop must be specified")
        elsif zstop && height
          raise("Either height or zstop must be specified, not both")
        end
        if height
          zstop = zstart + height
        end
        @current_z = zstop

        if ! layer
          raise("No layer specified")
        end

        info = D25ZInfo::new(layer, zstart, zstop, @display || display)
        @zstack << info

        return info

      end

    end
    
    def zz(*args, &block)

      begin

        display = D25Display::new
        @display = display

        args.each do |a|

          if a.is_a?(D25ZInfo)

            @zstack.each do |z|
              if z == a
                z.display = display
              end
            end

          elsif a.is_a?(Hash)

            a[:color] && display.set_color(a[:color])
            a[:frame] && display.set_frame(a[:frame])
            a[:fill] && display.set_fill(a[:fill])
            a[:like] && display.set_like(a[:like])

            if a[:name]
              display.name = a[:name].to_s
            end

            invalid_keys = a.keys.select { |k| ![ :fill, :frame, :color, :hollow, :like, :name ].member?(k) }
            if invalid_keys.size > 0
              raise("Keyword argument(s) not understood: #{invalid_keys.collect(&:to_s).join(',')}")
            end

          else
            raise("Argument not understood: #{a.inspect}")
          end
              
        end

        block && yield

      ensure
        @display = nil
      end

    end

    def _check

      if @zstack.empty?
        raise("No z calls made in 2.5d script")
      end

    end

    def _finish(final = true)

      super(final)

      if final

        view = RBA::LayoutView::current.open_d25_view

        begin

          view.begin(self._generator)

          displays = {}

          @zstack.each do |z|
            (displays[z.display.object_id] ||= []) << z
          end

          displays.each do |k,zz|
            display = zz[0].display
            view.open_display(display.frame, display.fill, display.like, display.name)
            zz.each do |z|
              view.entry(z.layer.data, self.dbu, z.zstart, z.zstop)
            end
            view.close_display
          end
          
          view.finish

        rescue => ex
          view.clear
          view.close
          raise ex
        end

      end

    end

  end
 
end

