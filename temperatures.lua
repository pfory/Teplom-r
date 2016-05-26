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

heartBeat = node.bootreason() + 10
print("Boot reason:")
print(heartBeat)

wifi.setmode(wifi.STATION)
wifi.sta.config("Datlovo","Nu6kMABmseYwbCoJ7LyG")
cfg={
  ip = "192.168.1.151",
  netmask = "255.255.255.0",
  gateway = "192.168.1.1"
}
wifi.sta.setip(cfg)
wifi.sta.autoconnect(1)

Broker="88.146.202.186"  


pin = 4 --GPIO2
t.setup(pin)
addrs=t.addrs()
devices = table.getn(addrs)

print("Found "..devices.." DS18B20 device(s) on "..pin.." pin.")

versionSW             = "0.4"
versionSWString       = "Temperatures v" 
print(versionSWString .. versionSW)

function sendData()
  --print(node.heap())
  --print(file.fsinfo())
  for i=1,devices,1 do
    print(t.read(addrs[i],t.C))
  end
  print("---------------------------------")
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
  
function reconnect()
  print ("Waiting for Wifi")
  if wifi.sta.status() == 5 and wifi.sta.getip() ~= nil then 
    print ("Wifi Up!")
    tmr.stop(1) 
    m:connect(Broker, 31883, 0, 1, function(conn) 
      print(wifi.sta.getip())
      print("Mqtt Connected to:" .. Broker) 
      mqtt_sub() --run the subscription function 
    end)
  end
  heartBeat=20
  sendHB()
end

function sendHB()
  print("I am sending HB to OpenHab")
  m:publish(base.."HeartBeat",   heartBeat,0,0)
 
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
m:on("offline", function(con)   
  print("Mqtt Reconnecting...")   
  tmr.alarm(1, 10000, 1, function()  
    reconnect()
  end)
end)  


uart.write(0,"Connecting to Wifi")
tmr.alarm(0, 1000, 1, function() 
  print ("Connecting to Wifi... ")
  if wifi.sta.status() == 5 and wifi.sta.getip() ~= nil then 
    uart.write(0,".")
    tmr.stop(0) 
    m:connect(Broker, 31883, 0, 1, function(conn) 
      mqtt_sub() --run the subscription function 
      print(wifi.sta.getip())
      print("Mqtt Connected to:" .. Broker.." - "..base) 
      m:publish(base.."VersionSW",   versionSW,0,0)  
      sendHB() 
      tmr.alarm(3, 60000, 1, function()  
        sendData()
      end)
    end)  
  end
end)