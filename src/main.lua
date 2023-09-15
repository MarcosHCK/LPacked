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

  function lpacked.main (args)
    local app = Lp.Application.new ()

    function app:on_activate ()
      if (self.exec) then
        app:open ({}, 'exec')
      elseif (self.pack) then
        local value

        do
          local chunk
          local env = { math = { huge = math.huge, } }

          do
            local file = Gio.File.new_for_commandline_arg (self.pack)
            local info = assert (file:query_info ('standard::size', 0))
            local stream = assert (file:read ())
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
              end, '=descriptor', 't', env))
          end

          value = not setfenv and chunk () or setfenv (chunk, env) ()
        end

        do
          -- Check fields
          local optional = { description = 'string', main = 'string', }
          local mandatory = { name = 'string', pack = 'table', }

          for field, type_ in ipairs (optional) do
            local got = type (desc [field])
            if (got ~= 'nil' and got ~= type_) then
              local prep = type_ == 'string' and 'an' or 'a'
              error (([[optional descriptor field '%s' must contain %s %s value]]):format(prep, type_))
            end
          end

          for field, type_ in ipairs (mandatory) do
            local got = type (desc [field])
            if (got == 'nil') then
              error (([[mandatory descriptor field '%s' is missing]]):format (field))
            elseif (got ~= type_) then
              local prep = type_ == 'string' and 'an' or 'a'
              error (([[mandatory descriptor field '%s' must contain %s %s value]]):format(prep, type_))
            end
          end
        end
      end
    end

    function app:on_open (files)
      if (self.pack) then
        log.critical ('--pack option does not takes any additional files')
      else
        error ('unimplemented')
      end
    end
  return app:run (args)
  end
return lpacked
end
