#!/usr/bin/ruby 

files = []

glyph_dir = File.join(File.dirname($0), "..", "src", "db", "db", "glyphs")
Dir::new(glyph_dir).each do |file|
  if file !~ /^\./
    files << file
  end
end

ccfile = File.join(File.dirname($0), "..", "src", "db", "db", "glyphs.cc")
File.open(ccfile, "w") do |out|

  out.puts <<"END"
/**
 *  THIS FILE HAS BEEN CREATED AUTOMATICALLY BY "compile_glyphs.rb"
 *  DO NOT EDIT!
 */
END

  nfile = 0

  files.each do |f|

    nfile += 1

    name = f.sub(/\..*$/, "")

    out.puts("\n// File: #{f}")
    out.puts("static const char *name_#{nfile} = \"#{name}\";")
    out.puts("static const char *description_#{nfile} = \"#{f}\";")
    out.puts("static const uint8_t data_#{nfile}[] = {");
    
    File.open(File.join(glyph_dir, f), "rb") do |ly|

      bytes = ly.read
      hex = ""
      bytes.size.times do |i|
        hex += "0x%02x, " % bytes[i].ord
        if i % 8 == 7
          out.puts "  " + hex
          i = 0
          hex = ""
        end
      end

      if hex != ""
        out.puts "  " + hex
      end

    end

    out.puts("  0xff  //  dummy")
    out.puts("};")

  end

  out.puts("\nstatic void load_glyphs (std::vector<db::TextGenerator> &generators)\n{\n")

  nfile.times do |n|

    out.puts("  generators.push_back (db::TextGenerator ());")
    out.puts("  generators.back ().load_from_data ((const char *) data_#{n + 1}, sizeof (data_#{n + 1}) - 1, name_#{n + 1}, description_#{n + 1});\n")

  end

  out.puts("}")

end

