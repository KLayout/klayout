
# %include b_inc.lym

begin
  puts f
rescue => ex
  ln = ex.backtrace[0].split(":")
  # NOTE: as the backtrace is a native Ruby feature, include file translation
  # does not happen. We need to do this explicitly here:
  puts ex.to_s + " in " + RBA::Macro::real_path(ln[0], ln[1].to_i) + ":" + RBA::Macro::real_line(ln[0], ln[1].to_i).to_s
end

