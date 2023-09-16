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
  local GLib = lgi.require ('GLib', '2.0')
  local serialization = require ('org.hck.lpacked.serialization')

  local function pack (file, output)
    local builder
    local desc

    do
      local chunk
      local env = { math = { huge = math.huge, } }

      do
        local info = assert (file:query_info ('standard::size', 0))
        local stream = assert (file:read ())
        local left = info:get_size ()
        local pre = true

        chunk = assert (load (function ()
            if (pre) then
              pre = false
              return 'return '
            end

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

      ---@diagnostic disable-next-line: deprecated
      desc = not setfenv and chunk () or setfenv (chunk, env) ()
    end

    do
      -- Check fields
      local optional = { description = 'string', main = 'string', }
      local mandatory = { name = 'string', pack = 'table', }

      for field, type_ in pairs (optional) do
        local got = type (desc [field])
        if (got ~= 'nil' and got ~= type_) then
          local prep = type_ == 'string' and 'an' or 'a'
          error (([[optional descriptor field '%s' must contain %s %s value]]):format(prep, type_))
        end
      end

      for field, type_ in pairs (mandatory) do
        local got = type (desc [field])
        if (got == 'nil') then
          error (([[mandatory descriptor field '%s' is missing]]):format (field))
        elseif (got ~= type_) then
          local prep = type_ == 'string' and 'an' or 'a'
          error (([[mandatory descriptor field '%s' must contain %s %s value]]):format(prep, type_))
        end
      end
    end

    builder = Lp.PackBuilder ()

    do
      local manifest =
        {
          description = desc.description,
          main = desc.main,
          name = desc.name,
        }

      local data = serialization.serialize (manifest)
      local bytes = GLib.Bytes (data)
      builder:add_from_bytes ("/manifest.lua", bytes)
    end

    for path, files in pairs (desc.pack) do
      if (type (path) ~= 'string' or type (files) ~= 'table') then
        error ([[descriptor field 'pack' table must contain string-table pairs]])
      else
        for alias, filename in pairs (files) do
          if (type (alias) == 'number') then
            alias = filename
          end

          local name = Lp.canonicalize_alias (path, alias)
          local file = Gio.File.new_for_commandline_arg (filename)
          local info = assert (file:query_info ('standard::size', 0))
          local stream = assert (file:read ())

          builder:add_from_stream (name, stream, info:get_size ())
        end
      end
    end

    do
      local filename = output or Lp.canonicalize_pack_name (desc.name)
      local file = Gio.File.new_for_commandline_arg (filename)
      local stream = assert (file:replace (nil, false, 'PRIVATE'))

      assert (builder:write_to_stream (stream))
      assert (stream:close ())
    end
  end
return pack
end
