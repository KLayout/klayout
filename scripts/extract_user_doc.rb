
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

title = "KLayout "
title += ($klayout_version || "$version") + " "
title += "("
title += ($klayout_version_date || "$date") + " "
title += ($klayout_version_rev || "$rev")
title += ") "
title += ($target_info && $target_info != "" ? "[#{$target_info}] " : "") 

HEADER=<<"END"
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
  <meta http-equiv="Content-Type" content="text/html; charset=utf-8">
  <title>KLayout Documentation</title>
  <link rel="stylesheet" type="text/css" href="format.css"/>
  <style>

body,p,h1,h2,h3,h4,li,ul,td,div,span {
  font-family: Verdana, Geneva, Arial, Helvetica, sans-serif; 
}

tt,pre {
  font-family: Lucidatypewriter, Andale Mono, Courier, monospace;
}

tt {
  font-weight: normal;
}

a:focus {
  outline: none;
  text-decoration: none;
}

a {
  color: #378eb2;
  font-weight: bold;
  text-decoration: none;
}

a:hover {
  color: #378eb2;
  font-weight: bold;
  text-decoration: none;
  border-bottom: solid;
  border-width: 1px;
}

a.img-link:hover {
  border: none;
}

a.img-link img {
  border-color: #378eb2;
  padding: 1px;
}

a.img-link:hover img {
  border-color: #378eb2;
  padding: 0px;
  border-width: 1px;
  border-style: solid;
}

a:visited {
  color: #378eb2;
  font-weight: bold;
  text-decoration: none;
}

body,p,ul,ol,td,li {
  font-size: 11pt;
}

large {
  font-size: 16pt;
}

small {
  font-size: 8pt;
}

p,img,h4,h3 {
  margin-top: 7pt;
  margin-bottom: 7pt;
}

pre,tt {
  font-size: 11pt;
}

table.body {
  table-layout: fixed;
  width: 100%;
}

td.content {
  background-color: white;
  vertical-align: top;
  padding: 0.5cm;
  position: absolute;
  margin: 0;
  left: 220px;
  right: 0;
  top: 0;
  bottom: 0;
  overflow: auto;
  color: #303030;
}

table.text {
  margin-left: 5mm;
}

td.text {
  padding: 2mm;
  vertical-align: top;
}

img.img-link,img.img-link:hover {
  border-width: 1px;
  border-color: #fff8c0;
  padding: 3px;
}

img.img-overview,img.img-overview:hover {
  border-width: 1px;
  border-color: #fff8c0;
  margin: 0;
}

td.img-title,p.img-title {
  border-bottom-width: 1px;
  border-bottom-color: black;
  border-bottom-style: solid;
  font-size: 12pt;
}

td.img-subscript,p.img-subscript {
  font-size: 11pt;
  color: black;
  margin-left: 0;
  margin-bottom: 2px;
  padding-bottom: 5px;
}

h1 {
  font-family: Impact, Charcoal, sans-serif;
  color: #b2844f;
  padding-right: 0.5cm;
  padding-top: 1mm;
  padding-bottom: 1mm;
  font-weight: normal;
  font-size: 32pt;
}

h2 {
  font-family: Palatino Linotype, Book Antiqua;
  border-bottom-width: 0.25mm;
  border-bottom-color: #b2844f;
  border-bottom-style: solid;
  padding-top: 3mm;
  padding-bottom: 1mm;
  font-weight: normal;
  font-size: 18pt;
  clear: both;
}

h3 {
  font-family: Palatino Linotype, Book Antiqua;
  padding-top: 2mm;
  padding-bottom: 1mm;
  font-weight: bold;
  font-size: 12pt;
}

h4 {
  padding-top: 2mm;
  padding-bottom: 1mm;
  margin-bottom: 0mm;
  font-weight: bold;
  font-size: 11pt;
}

  </style>
</head>
<body>
<small>#{title}</small>
END

TAIL=<<"END"
</body>
END


hs = RBA::HelpSource.new

hs.urls.each do |url|

  begin

    fn = ($target_doc + url).sub(/\.xml$/, ".html")

    t = hs.get(url)
    t = HEADER + xml2html(t) + TAIL

    tt = nil
    if File.exist?(fn) 
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


