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

    attr_accessor :fill, :frame, :like

    def initialize
      self.fill = nil
      self.frame = nil
      self.like = nil
    end

  end

  # The D25 engine
  
  class D25Engine < DRC::DRCEngine
  
    def initialize

      super

      @current_z = 0.0
      @zstack = []

      # clip to layout view
      if RBA::LayoutView::current
        self.clip(RBA::LayoutView::current.box)
      end

    end

    def z(*args)

      self._context("z") do

        layer = nil
        zstart = nil
        zstop = nil
        height = nil

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
            invalid_keys = a.keys.select { |k| ![ :height, :zstart, :zstop ].member?(k) }
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

        info = D25ZInfo::new(layer.data, zstart, zstop, @display || D25Display::new)
        @zstack << info

        return info

      end

    end
    
    def display(*args, &block)

      display = D25Display::new

      args.each do |a|

        if a.is_a?(D25ZInfo)

          @zstack.each do |z|
            if z == a
              z.display = display
            end
          end

        elsif a.is_a?(Hash)

          hollow_fill = 0xffffffff

          if a[:color]
            if !a[:color].is_a?(0xffffff.class)
              raise("'color' must be a color value (an integer)")
            end
            display.fill = a[:color]
            display.frame = a[:color]
          end
          if a[:frame]
            if !a[:frame].is_a?(0xffffff.class)
              raise("'frame' must be a color value (an integer)")
            end
            display.frame = a[:frame]
          end
          if a[:fill]
            if !a[:fill].is_a?(0xffffff.class)
              raise("'fill' must be a color value (an integer)")
            end
            display.fill = a[:fill]
          end
          if a[:hollow]
            if a[:hollow]
              display.fill = hollow_fill
            end
          end

          if a[:like]
            li = nil
            if a[:like].is_a?(String)
              li = RBA::LayerInfo::from_string(a[:like])
            elsif a[:like].is_a?(RBA::LayerInfo)
              li = a[:like]
            else
              raise("'like' must be a string or LayerInfo object")
            end
            display.like = li
          end

          invalid_keys = a.keys.select { |k| ![ :fill, :frame, :color, :hollow, :like ].member?(k) }
          if invalid_keys.size > 0
            raise("Keyword argument(s) not understood: #{invalid_keys.collect(&:to_s).join(',')}")
          end

        else
          raise("Argument not understood: #{a.inspect}")
        end
            
      end

    end

    def _finish(final = true)

      if final && @zstack.empty?
        raise("No z calls made in 2.5d script")
      end

      super(final)

      if final

        view = RBA::LayoutView::current.open_d25_view

        begin

          displays = {}

          @zstack.each do |z|
            (displays[z.display.object_id] ||= []) << z
          end

          displays.each do |k,zz|
            display = zz[0].display
            view.open_display(display.frame, display.fill, display.like)
            zz.each do |z|
              view.entry(z.layer, z.start, z.zstop)
            end
            view.close_display
          end
          
          view.finish

        rescue => ex
          view.close
          raise ex
        end

      end

    end

  end
 
end

