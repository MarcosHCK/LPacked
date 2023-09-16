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
  local exec = require ('org.hck.lpacked.exec')
  local pack = require ('org.hck.lpacked.pack')

  function lpacked.main (args)
    local app = Lp.Application.new ()

    function app:on_activate ()
      if (self.exec) then
        app:open ({}, 'exec')
      elseif (self.pack) then
        local file = Gio.File.new_for_commandline_arg (self.pack)
        local functor = function () return pack (file, self.pack_output) end
        local success, reason = xpcall (functor, lpacked.msghandler)

        if (not success) then
          log.critical (reason)
        end
      end
    end

    function app:on_open (files)
      if (self.pack) then
        log.critical ('--pack option does not takes any additional files')
      elseif (self.exec) then
        local functor = function () return exec (self.exec, files) end
        local success, reason = xpcall (functor, lpacked.msghandler)

        if (not success) then
          log.critical (reason)
        end
      end
    end
  return app:run (args)
  end
return lpacked
end
