local M = {}
local io = io
local pairs, print = pairs, print
local table, tostring, type = table, tostring, type
local setEncoding = setEncoding

function encMenu ()
  encodings = {'UTF-8', 'KOI8-R', 'CP1251', 'CP866'}
  menu = {}
  for k, v in pairs(encodings) do 
      table.insert(menu, {name = v; action = function () setEncoding(v) end})
  end
  return menu
end

function pairsByKeys(t, f)
    local a = {}
    for n in pairs(t) do table.insert(a, n) end
    table.sort(a, f)
    local i = 0      -- iterator variable
    local iter = function ()   -- iterator function
        i = i + 1
        if a[i] == nil then return nil
        else return a[i], t[a[i]]
        end
    end
    return iter
end

function printTable(tbl, indent)
    for k, v in pairsByKeys(tbl) do
        if type(v) == 'table' then
            print(indent..k..':')
            local_indent = indent..'  '
            printTable(v, local_indent)
        else
            print(indent..tostring(k)..'='..tostring(v))
        end
    end
end

function dumpToFile(func, file)
    io.output(io.open(file, 'w+'))
    callback = function (str) io.write(str..'\n') end
    func(callback)
    io.close()
end

M.encMenu = encMenu
M.pairsByKeys = pairsByKeys
M.printTable = printTable
M.dumpToFile = dumpToFile

return M
