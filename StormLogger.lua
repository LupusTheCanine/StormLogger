--[[
Input
Number channnel 1..32 - data to be logged
Boolean channel 1 - transmit data
Boolean channel 2 - reconnect 1 tick impulse will start reconnect sequence as long as controller is in CONNECTED_IDLE or NO_RESPONSE state
Number Property "Sampling frequency Hz" - sampling frequency, resulting sampling frequency might be higher if 60/frequency is not a natural number
Number Property "Data format" - number format 0-"%g" 1-"%.17g" 2-"%a" 3-Custom 
Text Property "Custom format string" - printf like decription of value format must take at most one value and be valid format string
Text Property "chanel n name" - channel name that will be put in CSV header line, n in 1..32
Boolean Property "chanel n" - channel active if true/on channel is enabled, n in 1..32
Output
Video current status sized for 2x2 screen
]]--
State="INIT"
PORT=18080
headerSent=false
channelEnabled={}
channelName={}
lastResp=""
channelCount=0
busy=false
lineCount=0
formats={}
formats[0]="%g"
formats[1]="%.17g"
formats[2]="%a"
formats[3]=property.getText("Custom format string")
counter=0
function onTick()
	if State=="CONNECTED_IDLE" and headerSent and input.getBool(1) then State="TRANSMIT" end
	if State=="INIT" and not busy then
		async.httpGet(PORT,"REQC")
		busy=true
		State="WAIT_FOR_CONNECTION"
		for i=1,32 do
			channelEnabled[i]=property.getBool(string.format("channel %d",i))
			if (channelEnabled[i])then channelCount=channelCount+1 end
			channelName[i]=(property.getText(string.format("channel %d name",i)) or "")
		end
	elseif State=="CONNECTED_IDLE" and not headerSent and not busy then
		State="SENDING_HEADER"
		msg="HEAD "..channelCount.."\n"
		for i=1,32 do
			if channelEnabled[i] then
				msg=msg..channelName[i].."\n"
			end
		end
		lastResp=msg.."\n---"
		async.httpGet(PORT,msg)
		busy=true
	elseif State=="TRANSMIT" or State=="TRANSMIT_LAST" then
		if not input.getBool(1) then State="TRANSMIT_LAST" end
		counter=(counter+1)%(math.max(1,math.floor(60/(property.getNumber("Sampling frequency Hz")))))
		if counter==0 then
		msg=msg.."LINE "..channelCount.."\n"
		for i=1,32 do
			if channelEnabled[i] then
				msg=msg..formats[property.getNumber("Data format")]:format(input.getNumber(i)).."\n"
			end
		end
		lineCount=lineCount+1
		lastResp=msg.."\n---"
			if not busy then 
			msg="DATA "..lineCount.."\n"..msg 
			if State=="TRANSMIT_LAST" then msg=msg.."END\n" end
			async.httpGet(PORT,msg)
			lineCount=0
			busy=true
			msg=""
			if State=="TRANSMIT_LAST" then State="CONNECTED_IDLE" end
			end
		end
	end
	if input.getBool(2) and (State=="NO_RESPONSE" or State=="CONNECTED_IDLE") then
		State="INIT"
		headerSent=false
		busy=false
		channelEnabled={}
		channelName={}
		channelCount=0
	end
end

function onDraw()
	if State=="NO_RESPONSE" then
	statusMsg="No connection"
	elseif State=="CONNECTED_IDLE" then
	statusMsg="Connected"
	elseif State=="TRANSMIT" then
	statusMsg="Transmitting"
	elseif State=="TRANSMIT_LAST" then
	statusMsg="Finishing\ntransmission"
	elseif State=="INIT" or State=="WAIT_FOR_CONNECTION" then
	statusMsg="Connecting"
	elseif State=="SENDING_HEADER" then
	statusMsg="Sending header"
	else statusMsg="Invalid state:\n"..Status
	end
    screen.drawTextBox(0,0,64,64,statusMsg,0,0)

end
function httpReply(port, request_body, response_body)

	lastResp=request_body.."\n---\n"..response_body.."\n---"
    if PORT~=port then return -1 end
	busy=false
    if request_body=="REQC" then
    	if response_body=="READY" then
    		State="CONNECTED_IDLE"
    	else
    		State="NO_RESPONSE"
    	end
    elseif request_body:sub(1,4)=="HEAD" then
    	if response_body=="READY" then
    		headerSent=true
			State="CONNECTED_IDLE"
    	else
			State="NO_RESPONSE"
		end
    elseif request_body:sub(1,4)=="DATA" then
    	if response_body=="READY" then
		else
			State="NO_RESPONSE"
		end
	end
end
