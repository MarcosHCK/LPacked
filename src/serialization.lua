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
local lib = {}

do
  local keyword =
    {
      ["and"]=true, ["break"]=true,
      ["do"]=true, ["else"]=true,
      ["elseif"]=true, ["end"]=true,
      ["false"]=true, ["for"]=true,
      ["function"]=true, ["goto"]=true,
      ["if"]=true, ["in"]=true,
      ["local"]=true, ["nil"]=true,
      ["not"]=true, ["or"]=true,
      ["repeat"]=true, ["return"]=true,
      ["then"]=true, ["true"]=true,
      ["until"]=true, ["while"]=true
    }

  local validid = '^[%a_][%w_]*$'

  function lib.serialize (value, pretty)
    local visiting = {}
    local function dovalue (value, level)
      local type_ = type (value)

      if (type_ == 'nil') then
        return type_
      elseif (type_ == 'number') then
        if (value ~= value) then
          return '0/0'
        elseif (value == math.huge) then
          return 'math.huge'
        elseif (value == -math.huge) then
          return '-math.huge'
        else
          return tostring (value)
        end
      elseif (type_ == 'string') then
        return ('%q'):format (value):gsub ('\\\n', '\n')
      elseif (type_ == 'boolean') then
        return value and 'true' or 'false'
      elseif (type_ == 'table') then
        if (visiting [value]) then
          if (pretty) then
            return '(recursive table value)'
          else
            error [[table with cycles are not supported]]
          end
        else
          local f, n = nil, 0
          visiting [value] = true

          if (not pretty) then
            f = { pairs (value), }
          else
            local number_keys = {}
            local string_keys = {}
            local other_keys = {}

            for key in pairs (value) do
              type_ = type (key)

              if (type_ == 'number') then number_keys [#number_keys + 1] = key
              elseif (type_ == 'string') then string_keys [#string_keys + 1] = key
              else other_keys [#other_keys + 1] = key
            end end

            table.sort (number_keys)
            table.sort (string_keys)

            for _, k in ipairs (number_keys) do other_keys [#other_keys + 1] = k end
            for _, k in ipairs (string_keys) do other_keys [#other_keys + 1] = k end

            f =
              {
                function ()
                  n = n + 1
                  local key = other_keys [n]

                  if (not key) then
                    return nil
                  else
                    return key, value [key]
                  end
                end
              }
          end

          local i, result = 1, nil

    ---@diagnostic disable-next-line: deprecated
          for key, value_ in (table.unpack or unpack) (f) do
            if (not result) then
              result = '{'
            else
              result = ('%s,%s'):format (result, (not pretty) and '' or ('\n' .. string.rep (' ', level)))
            end

            type_ = type (value_)

            if (type_ == 'number' and key == i) then
              i = i + 1
              result = result .. dovalue (value_, level + 1)
            else
              if (type_ == 'string' and not keyword [key] and key:match (validid)) then
                result = result .. key
              else
                result = ('%s[%s]'):format (result, dovalue (key, level + 1))
              end

              result = ('%s=%s'):format (result, dovalue (value_, level + 1))
            end
          end

          visiting [value] = false
          return (result or '{') .. '}'
        end
      end
    end

    local result = dovalue (value, 1)
    local limit = type (pretty) == 'number' and pretty or 10

    if (pretty) then
      local truncate = 0

      while (limit > 0 and truncate) do
        truncate = string.find (result, '\n', truncate + 1, true)
        limit = limit - 1
      end

      if (truncate) then
        return result:sub (1, truncate) .. '...'
      end
    end
  return result
  end

  function lib.unserialize (data, loader)
    local env = { math = { huge = math.huge, } }
---@diagnostic disable-next-line: deprecated, redundant-parameter
    local loader_ = loader or loadstring or load
    local chunk, reason = loader_ (data, '=unserialize', 't', env)

    if (not chunk) then
      return nil, reason
    else
---@diagnostic disable-next-line: deprecated
      if (setfenv) then
---@diagnostic disable-next-line: deprecated
        setfenv (chunk, env)
      end

      local succeeds, result = pcall (chunk)

      if (succeeds) then
        return result
      else
        return nil, result
      end
    end
  end
return lib
end
