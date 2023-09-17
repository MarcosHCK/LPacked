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

    builder.name = desc.name
    builder.description = desc.description

    local function addfile (alias, filename, prefix)
      if (type (alias) == 'number') then
        alias = filename
      elseif (type (alias) ~= 'string') then
        error ([[descriptor field 'pack' is malformed]])
      end

      local name = Lp.canonicalize_alias (prefix, alias)
      local file = Gio.File.new_for_commandline_arg (filename)
      local info = assert (file:query_info ('standard::size', 0))
      local stream = assert (file:read ())

      builder:add_from_stream (name, stream, info:get_size ())
    end

    local function addpack (table, prefix)
      for key, value in pairs (table) do
        if (type (value) == 'table') then
          local nextp = GLib.build_filenamev { prefix, key, '/' }
          local canon = GLib.canonicalize_filename (nextp, prefix)
          addpack (value, canon)
        elseif (type (value) == 'string') then
          addfile (key, value, prefix)
        else
          error ([[descriptor field 'pack' is malformed]])
        end
      end
    end

    addpack (desc.pack, '/')

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
