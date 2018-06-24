
# Produces the documentation text from within KLayout

def xml2html(t)

  # strip XML processing instructions and DTD
  t = t.sub(/^.*<body>/m, "")
  t = t.sub(/<\/body>.*$/m, "")

  # insert title in front of navigator
  t = t.sub(/<p class="navigator">/, "<p class=\"navigator\"><b>KLayout Documentation (Qt #{$qt}): </b>") 

  # replace .xml references in hrefs
  t = t.gsub(/href=\"(.*?)\.xml\"/) { "href=\"#{$1}.html\""; }
  t = t.gsub(/href=\"(.*?)\.xml#(.*?)\"/) { "href=\"#{$1}.html##{$2}\""; }

  # replace simplified XML tags with an opening/closing tag
  t = t.gsub(/<(\w+)([^<>]*?)\/>/) { "<#{$1}#{$2}></#{$1}>" }

  # replace some closing tags which are not allowed
  t = t.gsub(/<\/br>/, "").gsub(/<\/img>/, "")

  t

end


hs = RBA::HelpSource.new

hs.urls.each do |url|

  begin

    fn = ($target_doc + url).sub(/\.xml$/, ".html")

    t = hs.get(url)
    t = xml2html(t) 

    tt = nil
    if File.exists?(fn) 
      File.open(fn, "rb") { |f| tt = f.read }
    end

    if t != tt
      puts "Writing #{fn} .."
      File.open(fn, "wb") do |f|
        f.write(t)
      end
    else
      puts "Retained #{fn}."
    end

  rescue => ex
    puts "*** ERROR: #{ex.to_s}"
  end

end


