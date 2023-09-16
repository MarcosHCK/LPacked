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
    local manifest

    for _, file in ipairs (files) do
      reader:add_from_file (file)
    end

    assert (reader:contains (main), ('no such file \'%s\''):format (main))

    do
      local chunk
      local env = { math = { huge = math.huge, } }
          env = _G

      do
        local info = assert (reader:query_info (main, 'standard::size'))
        local stream = assert (reader:open (main))
        local left = info:get_size ()

        chunk = assert (load (function ()
            if (left == 0) then
              return nil
            else
              local bytes = stream:read_bytes (left)
              local done = bytes:get_size ()
                left = left - done
              return bytes:get_data ()
            end
          end, '=' .. main, 't', env))
      end

      ---@diagnostic disable-next-line: deprecated
      manifest = not setfenv and chunk () or setfenv (chunk, env) ()
    end
  end
return exec
end
