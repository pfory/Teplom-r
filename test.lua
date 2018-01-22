--nacteni konfigurace teplomeru
filename ="device.config"

tabSensorId = {}
tabMQTTSubstring = {}
tabSensorId[1] = "sensor1"
tabSensorId[2] = "sensor2"
tabMQTTSubstring[1] = "tLivingRoom"
tabMQTTSubstring[2] = "tBedRoomNew"
print(tabSensorId[1])
print(tabSensorId[2])
print(tabMQTTSubstring[1])
print(tabMQTTSubstring[2])


for i=1,table.getn(tabSensorId),1 do  
  print(tabSensorId[i]..";"..tabMQTTSubstring[i])
end

files = file.list()
if files[filename] then
  print("Config file exists, read DS18B20 configuration.") --format device id;mqtt substring
  file.open(filename, "r")
  
  while true do 
    line = file.readline() 
    if (line == nil) then break 
    end 
    print(string.sub(line, 1, -2)) 
    tmr.wdclr()
  end
  file.close()
else
  file.open(filename, "w")
  for i=1,table.getn(tabSensorId),1 do  
    file.writeline(tabSensorId[i]..";"..tabMQTTSubstring[i])
  end
  file.close()
  print("Config file does not exists. New file device.config was created.")
end


