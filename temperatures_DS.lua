--[[1 - loznice nova
2 - loznic stara
3 - strecha
4 - obyvak
5
6
7
8
]]--

t=require('ds18b20')
--Init  
base = "/home/bedNew/esp03/"
deviceID = "ESP8266 Temperatures "..node.chipid()

wifi.setmode(wifi.STATION)
wifi.sta.config("Datlovo","Nu6kMABmseYwbCoJ7LyG")

Broker="88.146.202.186"  

pin = 4 --GPIO2
t.setup(pin)
addrs=t.addrs()
devices = table.getn(addrs)

print("Found "..devices.." DS18B20 device(s) on "..pin.." pin.")

versionSW             = "0.2"
versionSWString       = "Central Heating v" 
print(versionSWString .. versionSW)

function sendData()
  --print(node.heap())
  --print(file.fsinfo())
  for i=1,devices,1 do
    print(t.read(addrs[i],t.C))
  end
  print("---------------------------------")
  --sec, usec = rtctime.get()
  --print(sec)
  --print(usec)
  print("I am sending data from "..deviceID.." to OpenHab")
  if t.read(addrs[4],t.C)~=nil then
    m:publish(base.."tLivingRoom", string.format("%.1f",t.read(addrs[4],t.C)),0,0)  
  end
  if t.read(addrs[1],t.C)~=nil then
    m:publish(base.."tBedRoomNew", string.format("%.1f",t.read(addrs[1],t.C)),0,0) 
  end
  if t.read(addrs[2],t.C)~=nil then
    m:publish(base.."tBedRoomOld", string.format("%.1f",t.read(addrs[2],t.C)),0,0) 
  end
  if t.read(addrs[5],t.C)~=nil then
    m:publish(base.."tCorridor", string.format("%.1f",t.read(addrs[5],t.C)),0,0) 
  end
  if t.read(addrs[6],t.C)~=nil then
    m:publish(base.."tHall", string.format("%.1f",t.read(addrs[6],t.C)),0,0) 
  end
  if t.read(addrs[7],t.C)~=nil then
    m:publish(base.."tBath", string.format("%.1f",t.read(addrs[7],t.C)),0,0) 
  end
  if t.read(addrs[8],t.C)~=nil then
    m:publish(base.."tWorkRoom", string.format("%.1f",t.read(addrs[8],t.C)),0,0) 
  end
  if t.read(addrs[3],t.C)~=nil then
    m:publish(base.."tAttic", string.format("%.1f",t.read(addrs[3],t.C)),0,0) 
  end
  m:publish(base.."VersionSW",              versionSW,0,0)  
  m:publish(base.."HeartBeat",              heartBeat,0,0)  
  if heartBeat==0 then heartBeat=1
  else heartBeat=0
  end
end

function mqtt_sub()  
  m:subscribe(base.."com",0, function(conn)   
    print("Mqtt Subscribed to OpenHAB feed for device "..deviceID)  
  end)  
end  

m = mqtt.Client(deviceID, 180, "datel", "hanka12")  
m:lwt("/lwt", deviceID, 0, 0)  


tmr.alarm(0, 1000, 1, function() 
  print ("Connecting to Wifi... ")
  if wifi.sta.status() == 5 and wifi.sta.getip() ~= nil then 
    print ("Wifi connected")
    tmr.stop(0) 
    m:connect(Broker, 31883, 0, function(conn) 
      mqtt_sub() --run the subscription function 
      print(wifi.sta.getip())
      print("Mqtt Connected to:" .. Broker.." - "..base) 
      sendData()
    end)
  end
end)