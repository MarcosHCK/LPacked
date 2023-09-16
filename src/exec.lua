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
local lgi = require('lgi')
local lpacked = require ('org.hck.lpacked')
local log = lgi.log.domain('LPacked')
local lp = lgi.package('lpacked')
local Lp = lgi.require('LPacked')

do
  local Gio = lgi.require ('Gio', '2.0')

  local function exec (main, files)
    local reader = Lp.PackReader ()
    local module

    for _, file in ipairs (files) do
      assert (reader:add_from_file (file))
    end

    assert (reader:contains (main), ('no such file \'%s\''):format (main))

    local function searchpath (name, path, sep, rep)
      local errors
      local fullpath

      errors = { '', }
      rep = rep or '/'
      sep = sep or '.'

      name = name:gsub ('%' .. sep, rep)

      for template in path:gmatch ('([^;]+)') do
        fullpath = template:gsub ('?', name)

        if (reader:contains (fullpath)) then
          return fullpath
        else
          table.insert (errors, ('\tno file \'%s\''):format (fullpath))
        end
      end
    return nil, table.concat (errors, '\n')
    end

    local function loadpath (name)
      local info = assert (reader:query_info (name, 'standard::size'))
      local stream = assert (reader:open (name))
      local left = info:get_size ()

      return load (function ()
          if (left == 0) then
            return nil
          else
            local bytes = stream:read_bytes (left)
            local done = bytes:get_size ()
              left = left - done
            return bytes:get_data ()
          end
        end, '=' .. name)
    end

    local function searcher (name)
      local fullpath, reason = searchpath (name, package.path)
      if (not fullpath) then
        return reason
      else
        local chunk, reason = loadpath (fullpath)
        return chunk or reason
      end
    end

    do
      package.path = '/?.lua;/?/init.lua'
      package.cpath = package.cpath

      local loaded = package.loaded
    ---@diagnostic disable-next-line: deprecated
      local loaders = package.loaders or package.searchers

      for key in pairs (loaded) do
        if (not key:match ('^lgi')) then
          loaded [key] = nil
        end
      end

      for key in ipairs (loaders) do
        if (key > 1) then
          loaders [key] = nil
        end
      end

      table.insert (loaders, searcher)
    end
  return assert (loadpath (main)) ()
  end
return exec
end
