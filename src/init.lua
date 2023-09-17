--[[
-- Copyright 2021-2025 MarcosHCK
-- This file is part of LPacked.
--
-- LPacked is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- LPacked is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with LPacked.  If not, see <http://www.gnu.org/licenses/>.
]]
local lgi = require ('lgi')
local log = lgi.log.domain ('LPacked')
local lpacked = {}

do
  local Gio = lgi.require ('Gio', '2.0')

  local function searchpath (name, path, sep, rep)
    local bytes
    local errors
    local fullpath
    local reason

    errors = { '', }
    rep = rep or '/'
    sep = sep or '.'

    name = name:gsub ('%' .. sep, rep)

    for template in path:gmatch ('([^;]+)') do
      fullpath = template:gsub ('?', name)
      bytes, reason = Gio.resources_lookup_data (fullpath, 0)

      if (bytes) then
        return bytes, fullpath
      else
        if (reason:matches (Gio.ResourceError, Gio.ResourceError.NOT_FOUND)) then
          table.insert (errors, ('\tno resource \'%s\''):format (fullpath))
        else
          error (reason)
        end
      end
    end
  return nil, table.concat (errors, '\n')
  end

  local function searcher (name)
    local bytes, result = searchpath (name, '/?.lua;/?/init.lua')

    if (not bytes) then
      return result
    else
      local chunkname = '=' .. result
---@diagnostic disable-next-line: deprecated
      local chunk, reason = (loadstring or load) (bytes:get_data (), chunkname)
      return chunk or reason
    end
  end

---@diagnostic disable-next-line: deprecated
  table.insert (package.searchers or package.loaders, searcher)
  package.loaded ['org.hck.lpacked'] = lpacked
return require ('org.hck.lpacked.main')
end
