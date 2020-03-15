-- Creating dirs in case they do not exist
System.createDirectory("ux0:/data/TrackPlug")
System.createDirectory("ux0:/data/TrackPlugArchive")

-- Scanning TrackPlug folder
local tbl = System.listDirectory("ux0:/data/TrackPlug")
local blacklist = {
    --"VITASHELL", -- Vitashell
    --"TPLG00001", -- TrackPlug
    "NPXS10028", -- PSPEMU app itself
    "NPXS10083"
}
-- Removing blacklisted games
for i, file in pairs(tbl) do
	local titleid = string.sub(file.name,1,-5)
    for k, toberemoved in pairs(blacklist) do
	    if titleid == blacklist[k] then
            System.deleteFile("ux0:/data/TrackPlug/"..file.name)
        end
    end
end
-- Reset the table
tbl = System.listDirectory("ux0:/data/TrackPlug")

if tbl == nil then
    tbl = {}
end

-- Convert a 32 bit binary string to an integer
function bin2int(str)
    local b1, b2, b3, b4 = string.byte(str, 1, 4)
    return bit32.lshift(b4, 24) + bit32.lshift(b3, 16) + bit32.lshift(b2, 8) + b1
end

-- Format raw time data
function FormatTime(val)
    local minutes = math.floor(val/60)
    local seconds = val%60
    local hours = math.floor(minutes/60)
    local minutes = minutes%60
    local res = ""
    if hours > 0 then
        res = hours .. "h "
    end
    if minutes > 0 then
        res = res .. minutes .. "m "
    end
    res = res .. seconds .. "s "
    return res
end

-- Recover title from homebrew database
function recoverTitle(tid)
    local file = System.openFile("ux0:/data/TrackPlugArchive/" .. tid .. ".txt", FREAD)
    fsize = System.sizeFile(file)
    local title = System.readFile(file, fsize)
    System.closeFile(file)
    return title
end

-- Extracts title name from an SFO file
function extractTitle(file, tid)
    local data = System.extractSfo(file)
    if System.doesFileExist("ux0:/data/TrackPlugArchive/" .. tid .. ".txt") then
        System.deleteFile("ux0:/data/TrackPlugArchive/" .. tid .. ".txt")
    end
    local file = System.openFile("ux0:/data/TrackPlugArchive/" .. tid .. ".txt", FCREATE)
    System.writeFile(file, data.title, string.len(data.title))
    System.closeFile(file)
    return data.title
end

function getRegion(titleid)
    local regioncode = string.sub(titleid,1,4)
    local prefix = string.sub(regioncode,1,2)
    local region = "Unknown"

    -- PSV common
    if regioncode == "PCSA" or regioncode == "PCSE" then
        region = "USA"
    elseif regioncode == "PCSB" or regioncode == "PCSF" then
        region = "Europe"
    elseif regioncode == "PCSC" or regioncode == "PCSG" then
        region = "Japan"
    elseif regioncode == "PCSD" or regioncode == "PCSH" then
        region = "Asia"
    -- Physical & NP releases (PSV/PSP/PS1)
    elseif prefix == "VC" or prefix == "VL" or
            prefix == "UC" or prefix == "UL" or
            prefix == "SC" or prefix == "SL" or
            prefix == "NP" then
        n1 = string.sub(regioncode,1,1)
        n3 = string.sub(regioncode,3,3)
        n4 = string.sub(regioncode,4,4)
        if n3 == "A" then
            region = "Asia"
        elseif n3 == "C" then
            region = "China"
        elseif n3 == "E" then
            region = "Europe"
        elseif n3 == "H" then
            region = "Hong Kong"
        elseif n3 == "J" or n3 == "P" then
            region = "Japan"
        elseif n3 == "K" then
            region = "Korea"
        elseif n3 == "U" then
            region = "USA"
        end

        if n1 == "S" then
            region = region .. " (PS1)"
        elseif n1 == "U" or
                (prefix == "NP" and (n4 == "G" or n4 == "H")) then
            region = region .. " (PSP)"
        elseif prefix == "NP" then
            if n4 == "E" or n4 == "F" then
                region = region .. " (PS1 - PAL)"
            elseif n4 == "I" or n4 == "J" then
                region = region .. " (PS1 - NTSC)"
            end
        end
    elseif prefix == "PE" then
        region = "Europe (PS1)"
    elseif prefix == "PT" then
        region = "Asia (PS1)"
    elseif prefix == "PU" then
        region = "USA (PS1)"
    elseif string.sub(titleid,1,6) == "PSPEMU" then
        region = "PSP/PS1"
    end

    return region
end

-- Loading unknown icon
local unk = Graphics.loadImage("app0:/unk.png")

-- Getting region, playtime, icon and title name for any game
for i, file in pairs(tbl) do
    if file.name == "config.lua" then
        dofile("ux0:/data/TrackPlug/"..file.name)
        cfg_idx = i
    else
        local titleid = string.sub(file.name,1,-5)
        file.region = getRegion(titleid)

        if System.doesFileExist("ur0:/appmeta/" .. titleid .. "/icon0.png") then
            file.icon = Graphics.loadImage("ur0:/appmeta/" .. titleid .. "/icon0.png")
        else
            file.icon = unk
        end
        if System.doesFileExist("ux0:/data/TrackPlugArchive/" .. titleid .. ".txt") then
            file.title = recoverTitle(titleid)
        elseif System.doesFileExist("ux0:/app/" .. titleid .. "/sce_sys/param.sfo") then
            file.title = extractTitle("ux0:/app/" .. titleid .. "/sce_sys/param.sfo", titleid)
        else
            file.title = "Unknown - " .. titleid
        end
        file.id = titleid
        fd = System.openFile("ux0:/data/TrackPlug/" .. file.name, FREAD)
        file.rtime = bin2int(System.readFile(fd, 4))
        file.ptime = FormatTime(file.rtime)
        System.closeFile(fd)
    end
end

-- Background wave effect
local colors = {
	{Color.new(72,72,72), Color.new(30,20,25), Color.new(200,180,180)}	-- Black'N'Rose
}
if col_idx == nil then
	col_idx = 0
end

local function LoadWave(height,dim,f,x_dim)
    f=f or 0.1
    local onda={pi=math.pi,Frec=f,Long_onda=dim,Amplitud=height}
    function onda:color(a,b,c) self.a=a self.b=b self.c=c end
    function onda:init(desfase)
        desfase=desfase or 0
        if not self.contador then
            self.contador=Timer.new()
        end
        if not self.a or not self.b or not self.c then
            self.a = 255
            self.b = 200
            self.c = 220
        end
        local t,x,y,i
        t = Timer.getTime(self.contador)/1000+desfase
        for x = 0,x_dim,8 do
			y = 404+self.Amplitud*math.sin(2*self.pi*(t*self.Frec-x/self.Long_onda))
            i = self.Amplitud*(self.pi/self.Long_onda)*math.cos(2*self.pi*(t*self.Frec-x/self.Long_onda))
			k = self.Amplitud*(1*self.pi/self.Long_onda)*math.sin(-1*self.pi*(t*self.Frec-x/self.Long_onda))
            Graphics.drawLine(x-30,x+30,y-i*30,y+i*30,Color.new(self.a,self.b,self.c,math.floor(x/65)))
			Graphics.drawLine(x-150,x+150,y-k*150,y+k*150,Color.new(self.a-60,self.b-80,self.a-70,math.floor(x/20)))
		end
    end
    function onda:destroy()
        Timer.destroy(self.contador)
    end
    return onda
end

wav = LoadWave(100,1160, 0.1, 1160)

-- Internal stuffs
local list_idx = 1
local order_idx = 1
local orders = {"Name", "Playtime"}

-- Ordering titles
table.sort(tbl, function (a, b) return (a.rtime > b.rtime ) end)

-- Internal stuffs
local white = Color.new(255, 255, 255)
local yellow = Color.new(255, 255, 0)
local grey = Color.new(40, 40, 40)

-- Shows an alarm with selection on screen
local alarm_val = 128
local alarm_decrease = true
function showAlarm(title, select_idx)
    if alarm_decrease then
        alarm_val = alarm_val - 4
        if alarm_val == 40 then
            alarm_decrease = false
        end
    else
        alarm_val = alarm_val + 4
        if alarm_val == 128 then
            alarm_decrease = true
        end
    end
    local sclr = Color.new(alarm_val, alarm_val, alarm_val)
    Graphics.fillRect(200, 760, 200, 280, grey)
    Graphics.debugPrint(205, 205, title, yellow)
    Graphics.fillRect(200, 760, 215 + select_idx * 20, 235 + select_idx * 20, sclr)
    Graphics.debugPrint(205, 235, "Yes", white)
    Graphics.debugPrint(205, 255, "No", white)
end
-- Scroll-list Renderer
local sel_val = 128
local decrease = true
local freeze = false
local mov_y = 0
local mov_step = 0
local new_list_idx = nil
local real_i = 1
local big_tbl = {}
function RenderList()
	local r_max = 0
	local r = 0
	if #tbl < 4 then
		r_max = 8
	else
		r_max = 2
	end
	while r < r_max do
		for k, v in pairs(tbl) do
			table.insert(big_tbl, v)
		end
		r = r + 1
	end
	local y = -124
	local i = list_idx - 1
	if not freeze then
		if decrease then
			sel_val = sel_val - 4
			if sel_val == 0 then
				decrease = false
			end
		else
			sel_val = sel_val + 4
			if sel_val == 128 then
				decrease = true
			end
		end
	end
    if mov_y ~= 0 then
        if math.abs(mov_y) < 104 then
		    mov_y = math.floor(mov_y*1.298)
        else
			mov_y = 0
		    list_idx = new_list_idx
            i = new_list_idx - 1
		end
	end
	while i <= list_idx + 4 do
		if i < 1 then
			real_i = i
			i = #big_tbl - math.abs(i)
		end
		Graphics.fillRect(5, 955, y+mov_y, y+mov_y-4, Color.new(255, 255, 255, 60))
		if i ~= list_idx + 5 then
			Graphics.drawImage(5, y + mov_y, big_tbl[i].icon)
		end
		Graphics.debugPrint(150, y + 35 + mov_y, big_tbl[i].title, Color.new(230,140,175))
		--Graphics.debugPrint(150, y + 45 + mov_y, "Title ID: " .. big_tbl[i].id, white)
		--Graphics.debugPrint(150, y + 65 + mov_y, "Region: " .. big_tbl[i].region, white)
		Graphics.debugPrint(150, y + 65 + mov_y, "Playtime: " .. big_tbl[i].ptime, white)
		local r_idx = i % #tbl
		if r_idx == 0 then
			r_idx = #tbl
		end
		Graphics.debugPrint(920, y + 100 + mov_y, "#" .. r_idx, white)
		y = y + 132
		if real_i <= 0 then
			i = real_i
			real_i = 1
		end
		i = i + 1
	end
end

-- Main loop
local f_idx = 1
local useless = 0
local oldpad = Controls.read()
while #tbl > 0 do
	Graphics.initBlend()
	Graphics.fillRect(0,960,0,544,Color.new(10,5,15))
	wav:init()
	RenderList()
	if freeze then
		showAlarm("Do you want to delete this record permanently?", f_idx)
	end
	Graphics.termBlend()
	Screen.flip()
	Screen.waitVblankStart()
    local pad = Controls.read()
	if Controls.check(pad, SCE_CTRL_UP) and mov_y == 0 then
		if freeze then
			f_idx = 1
		else
			new_list_idx = list_idx - 1
			if new_list_idx == 0 then
				new_list_idx = #tbl
			end
			mov_y = 11
		end
	elseif Controls.check(pad, SCE_CTRL_DOWN) and mov_y == 0 then
		if freeze then
			f_idx = 2
		else
			new_list_idx = list_idx + 1
			if new_list_idx > #tbl then
				new_list_idx = 1
			end
			mov_y = -11
		end
	elseif Controls.check(pad, SCE_CTRL_TRIANGLE) and not Controls.check(oldpad, SCE_CTRL_TRIANGLE) and not freeze then
		freeze = true
		f_idx = 1
	elseif Controls.check(pad, SCE_CTRL_CROSS) and not Controls.check(oldpad, SCE_CTRL_CROSS) and freeze then
		freeze = false
		if f_idx == 1 then -- Delete
			System.deleteFile("ux0:/data/TrackPlug/" .. tbl[list_idx].name)
			table.remove(tbl, list_idx)
			big_tbl = {}
			list_idx = list_idx - 1
        end
	end
	oldpad = pad
end

-- No games played yet apparently
while true do
    Graphics.initBlend()
    Screen.clear()
    Graphics.debugPrint(5, 5, "No games tracked yet.", white)
    Graphics.termBlend()
    Screen.flip()
    Screen.waitVblankStart()
end
