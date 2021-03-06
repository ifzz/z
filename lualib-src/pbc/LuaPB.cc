
#include "LuaPB.h"
#include "ProtoImporter.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/importer.h>

static int pb__repeated_add(lua_State* L);
static int pb_repeated_len(lua_State* L);
static const struct luaL_Reg repeatedlib[] =
{
		{"add", pb__repeated_add},
		{"len", pb_repeated_len},
		{NULL, NULL}
};

static lua_Alloc lua_alloc = NULL;

static int push_repeated_msg(lua_State* L, google::protobuf::Message* msg, google::protobuf::FieldDescriptor *field)
{
	lua_pushlightuserdata(L, field);
	lua_gettable(L, LUA_REGISTRYINDEX);
	if (lua_isnil(L, -1))
	{
		lua_newtable(L);

		lua_repeated_msg* repeated = static_cast<lua_repeated_msg*>(lua_newuserdata(L, sizeof(lua_repeated_msg)));
		repeated->msg = msg;
		repeated->field = field;
		lua_rawseti(L, -2, 0); //

		luaL_newlibtable(L, repeatedlib);
		lua_pushvalue(L, -1);
		luaL_setfuncs(L, repeatedlib, 1);

		luaL_getmetatable(L, PB_REPEATED_MESSAGE_META);
		lua_setmetatable(L, -2);

		lua_pushlightuserdata(L, field);
		lua_pushvalue(L, -2);
		lua_settable(L, LUA_REGISTRYINDEX);
	}
	return 1;
}

lua_repeated_msg* get_repeated_msg(lua_State* L)
{
	luaL_checktype(L, lua_upvalueindex(1), LUA_TTABLE);
	lua_rawgeti(L, lua_upvalueindex(1), 0);
	if (!lua_isnil(L, -1))
	{
		lua_repeated_msg* msg = static_cast<lua_repeated_msg*>(lua_touserdata(L, -1));
		return msg;
	}
	luaL_argerror(L, (0), "get_repeated_msg(lua_State* L), repeated msg is nill\n");
	return NULL;
}

lua_repeated_msg* get_repeated_msg(lua_State* L, int stackindex)
{
	luaL_checktype(L, stackindex, LUA_TTABLE);
	lua_rawgeti(L, stackindex, 0);
	if (!lua_isnil(L, -1))
	{
		lua_repeated_msg* msg = static_cast<lua_repeated_msg*>(lua_touserdata(L, -1));
		return msg;
	}
	luaL_argerror(L, (0), "get_repeated_msg(lua_State* L, int stackindex), repeated msg is nill\n");
	return NULL;
}

static int pb__repeated_add(lua_State* L)
{
	lua_repeated_msg* repeated = get_repeated_msg(L);
    google::protobuf::Message* message = repeated->msg;
    if (!message)
    {
    	luaL_argerror(L, 1, "pb__repeated_add, pb msg is nil");
    	return 0;
    }
    google::protobuf::FieldDescriptor* field = repeated->field;
    const google::protobuf::Reflection* reflection = message->GetReflection();
    luaL_argcheck(L, field != NULL, 1, "pb_repeated_add, field is null");

    if(field->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE)
   {
    	google::protobuf::Message* msg = reflection->AddMessage(message, field);
    	push_message(L, msg);
    	return 1;
    }
    else if(field->type() == google::protobuf::FieldDescriptor::TYPE_INT32)
    {
    	int val = static_cast<int>(luaL_checkinteger(L, 1));
        reflection->AddInt32(message, field, val);
    }
    else if (field->type() == google::protobuf::FieldDescriptor::TYPE_INT64)
    {
        long val = static_cast<long>(luaL_checknumber(L, 1));
        reflection->AddInt64(message, field, val);
    }
    else if(field->type() == google::protobuf::FieldDescriptor::TYPE_UINT32)
    {
        unsigned int val = static_cast<unsigned int>(luaL_checknumber(L, 1));
        reflection->AddUInt32(message, field, val);
    }
    else if (field->type() == google::protobuf::FieldDescriptor::TYPE_UINT64)
    {
        unsigned long val = static_cast<unsigned long>(luaL_checknumber(L, 1));
        reflection->AddUInt64(message, field, val);
    }
    else if(field->type() == google::protobuf::FieldDescriptor::TYPE_FLOAT)
    {
        float val = static_cast<float>(luaL_checknumber(L, 1));
        reflection->AddFloat(message, field, val);
    }
    else if(field->type() == google::protobuf::FieldDescriptor::TYPE_DOUBLE)
    {
        double val =  static_cast<double>(luaL_checknumber(L, 1));
        reflection->AddDouble(message, field, val);
    }
    else if(field->type() == google::protobuf::FieldDescriptor::TYPE_BOOL)
    {
        int val = static_cast<int>(luaL_checkinteger(L, 1));
        reflection->AddBool(message, field, val);
    }
    else if(field->type() == google::protobuf::FieldDescriptor::TYPE_STRING)
    {
    	size_t strlen;
    	const char *str = luaL_checklstring(L, 1, &strlen);
        reflection->AddString(message, field, str);
    }
    else if (field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES)
    {
    	size_t strlen;
    	const char *str = luaL_checklstring(L, 1, &strlen);
        reflection->AddString(message, field, str);
    }
    else
    {
        luaL_argerror(L, (1), "pb_repeated_add field name type for add  is not support!!");
     }
    return 0;
}

static int pb_repeated_get(lua_State* L)
{
	lua_repeated_msg* repeated = get_repeated_msg(L, 1);
    google::protobuf::Message* message = repeated->msg;
    if (!message)
    {
    	luaL_argerror(L, 1, "pb_repeated_get, pb msg is nil");
    	return 0;
    }
    google::protobuf::FieldDescriptor *field = repeated->field;
    const google::protobuf::Reflection* reflection = message->GetReflection();
    luaL_argcheck(L, field != NULL, 1, "pb_repeated_get field not exist");

    // -1 为了和lua的下标从一开始保持一致
	int index = static_cast<int>(luaL_checkinteger(L, 2)) - 1;
	luaL_argcheck(L, index >= 0, 2, "pb_repeated_get index expected >= 1");
	if(field->type() == google::protobuf::FieldDescriptor::TYPE_INT32)
	{
		lua_pushinteger(L, reflection->GetRepeatedInt32(*message, field, index));
	}
	else if(field->type() == google::protobuf::FieldDescriptor::TYPE_STRING)
	{
		lua_pushstring(L, reflection->GetRepeatedString(*message, field, index).data());
	}
    else if (field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES)
    {
		lua_pushstring(L, reflection->GetRepeatedString(*message, field, index).data());
    }
	else if(field->type() == google::protobuf::FieldDescriptor::TYPE_UINT32)
	{
		lua_pushinteger(L, reflection->GetRepeatedUInt32(*message, field, index));
	}
	else if(field->type() == google::protobuf::FieldDescriptor::TYPE_FLOAT)
	{
		lua_pushnumber(L, reflection->GetRepeatedFloat(*message, field, index));
	}
	else if(field->type() == google::protobuf::FieldDescriptor::TYPE_DOUBLE)
	{
		lua_pushnumber(L, reflection->GetRepeatedDouble(*message, field, index));
	}
	else if(field->type() == google::protobuf::FieldDescriptor::TYPE_BOOL)
	{
		lua_pushboolean(L, reflection->GetRepeatedBool(*message, field, index));
	}
	else if(field->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE)
	{
		google::protobuf::Message* msg = reflection->MutableRepeatedMessage(message, field, index);
		push_message(L, msg);
	}
	else
	{
		luaL_argerror(L, 0, "pb_repeated_get, field type for get not support!!!");
		return 0;
	}
	return 1;
}

static int pb_repeated_set(lua_State* L)
{
	lua_repeated_msg* repeated = get_repeated_msg(L, 1);
    google::protobuf::Message *message = repeated->msg;
    if (!message)
    {
    	luaL_argerror(L, 1, "pb_repeated_set, pb msg is nil");
    	return 0;
    }

    const google::protobuf::Reflection* reflection = message->GetReflection();
    google::protobuf::FieldDescriptor *field = repeated->field;
    luaL_argcheck(L, field != NULL, 1, "pb_repeated_set field not exist");

	int index = static_cast<int>(luaL_checkinteger(L, 2)) - 1;
	luaL_argcheck(L, index >= 0, 2, "pb_repeated_set index expected >= 1");

	if(field->type() == google::protobuf::FieldDescriptor::TYPE_INT32)
	{
		int val = static_cast<int>(luaL_checkinteger(L, 3));
		reflection->SetRepeatedInt32(message, field, index, val);
	}
	else if(field->type() == google::protobuf::FieldDescriptor::TYPE_UINT32)
	{
		unsigned int val = static_cast<unsigned int>(luaL_checkinteger(L, 3));
		reflection->SetRepeatedUInt32(message, field, index, val);
	}
	else if(field->type() == google::protobuf::FieldDescriptor::TYPE_FLOAT)
	{
		float val = static_cast<float>(luaL_checknumber(L, 3));
		reflection->SetRepeatedFloat(message, field, index, val);
	}
	else if(field->type() == google::protobuf::FieldDescriptor::TYPE_DOUBLE)
	{
		double val = static_cast<double>(luaL_checknumber(L, 3));
		reflection->SetRepeatedDouble(message, field, index, val);
	}
	else if(field->type() == google::protobuf::FieldDescriptor::TYPE_BOOL)
	{
		int val = static_cast<int>(lua_toboolean(L, 3));
		reflection->SetRepeatedBool(message, field, index, val);
	}
	else if(field->type() == google::protobuf::FieldDescriptor::TYPE_STRING)
	{
		size_t strlen;
		const char *str = static_cast<const char *>(luaL_checklstring(L, 3, &strlen));
		reflection->SetRepeatedString(message, field, index, str);
	}
	else if(field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES)
	{
		size_t strlen;
		const char *str = static_cast<const char *>(luaL_checklstring(L, 3, &strlen));
		reflection->SetRepeatedString(message, field, index, str);
	}
	else
	{
		luaL_argerror(L, (2), "pb_repeated_set type for set not support!!!");
	}
	return 0;
}

static int pb_repeated_len(lua_State* L)
{
	lua_repeated_msg* repeated = get_repeated_msg(L);
    google::protobuf::Message *message = repeated->msg;
    if (!message)
    {
    	luaL_argerror(L, 1, "pb_repeated_len, pb msg is nil");
    	return 0;
    }

    const google::protobuf::Reflection* reflection = message->GetReflection();
    google::protobuf::FieldDescriptor *field = repeated->field;
    luaL_argcheck(L, field != NULL, 1, "pb_repeated_len field not exist");

    int fieldsize = reflection->FieldSize(*message, field);
    lua_pushinteger(L, fieldsize);
    return 1;
}
////////////////////////////////////////////////////////////
static int pb_import(lua_State* L)
{
	const char* filename = luaL_checkstring(L, 1);
	sProtoImporter.Import(filename);
	return 0;
}

static int pb_new(lua_State* L)
{
	const char* type_name = luaL_checkstring(L, 1);
	google::protobuf::Message *message = sProtoImporter.createDynamicMessage(type_name);
	if (!message)
	{
		fprintf(stderr, "pb_new error, result is typename(%s) not found!\n", type_name);
		return 0;
	}

	push_message(L, message, true);
	return 1;
}

static int pb_delete(lua_State* L)
{
	lua_pbmsg* luamsg = (lua_pbmsg*)luaL_checkudata(L, 1, PB_MESSAGE_META);

    if (luamsg->isDelete && luamsg->msg)
    {
    	google::protobuf::Message*message = luamsg->msg;
    	delete message;
    	luamsg->msg = NULL;
    }
    return 0;
}

static int pb_tostring(lua_State* L)
{
	lua_pbmsg* luamsg = (lua_pbmsg*)luaL_checkudata(L, 1, PB_MESSAGE_META);
    google::protobuf::Message *message = luamsg->msg;
	if (!message)
	{
		luaL_argerror(L, 1, "pb_tostring,  pb msg is nil");
		return 0;
	}
    std::string msg(message->DebugString());
    lua_pushlstring(L, msg.c_str(), msg.length());
	return 1;
}

static int pb_get(lua_State* L)
{
	lua_pbmsg* luamsg = (lua_pbmsg*)luaL_checkudata(L, 1, PB_MESSAGE_META);
	const char* field_name = luaL_checkstring(L, 2);

    google::protobuf::Message *message = luamsg->msg;
    if (!message)
    {
    	luaL_argerror(L, 1, "pb_get,  pb msg is nil");
    	return 0;
    }

    const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
    const google::protobuf::Reflection* reflection = message->GetReflection();
    const google::protobuf::FieldDescriptor *field = descriptor->FindFieldByName(field_name);
    luaL_argcheck(L, (field != NULL), 2, "pb_get, field_name error");

    if (field->is_repeated())
    {
    	push_repeated_msg(L, message, const_cast<google::protobuf::FieldDescriptor *>(field));
    }
    else if(field->type() == google::protobuf::FieldDescriptor::TYPE_INT32)
	{
		lua_pushinteger(L, reflection->GetInt32(*message, field));
	}
    else if (field->type() == google::protobuf::FieldDescriptor::TYPE_INT64)
    {
        lua_pushnumber(L, reflection->GetInt64(*message, field));
    }
	else if(field->type() == google::protobuf::FieldDescriptor::TYPE_UINT32)
	{
		lua_pushinteger(L, reflection->GetUInt32(*message, field));
	}
    else if (field->type() == google::protobuf::FieldDescriptor::TYPE_UINT64)
    {
        lua_pushnumber(L, reflection->GetUInt64(*message, field));
    }
	else if(field->type() == google::protobuf::FieldDescriptor::TYPE_FLOAT)
	{
		 lua_pushnumber(L, reflection->GetFloat(*message, field));
	}
	else if(field->type() == google::protobuf::FieldDescriptor::TYPE_DOUBLE)
	{
		 lua_pushnumber(L, reflection->GetDouble(*message, field));
	}
	else if(field->type() == google::protobuf::FieldDescriptor::TYPE_BOOL)
	{
		lua_pushboolean(L, reflection->GetBool(*message, field));
	}
	else if(field->type() == google::protobuf::FieldDescriptor::TYPE_STRING)
	{
		std::string str(reflection->GetString(*message, field));
		lua_pushlstring(L, str.c_str(), str.length());
	}
	else if(field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES)
	{
		std::string str(reflection->GetString(*message, field));
		lua_pushlstring(L, str.c_str(), str.length());
	}
	else if(field->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE)
	{
    	google::protobuf::Message* msg = reflection->MutableMessage(message, field);
    	push_message(L, msg, false);
	}
    return 1;
}

static int pb_set(lua_State* L)
{
	lua_pbmsg* luamsg = (lua_pbmsg*)luaL_checkudata(L, 1, PB_MESSAGE_META);
	const char* field_name = luaL_checkstring(L, 2);

    google::protobuf::Message *message = luamsg->msg;
    if (!message)
    {
    	luaL_argerror(L, 1, "pb_set, pb msg is nil");
    	return 0;
    }

    const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
    const google::protobuf::Reflection* reflection = message->GetReflection();
    const google::protobuf::FieldDescriptor *field = descriptor->FindFieldByName(field_name);

    luaL_argcheck(L, field != NULL, 2, "LuaPB::set field_name error");
    luaL_argcheck(L, !field->is_repeated(), 2, "LuaPB::set field_name is repeated");

    if(field->type() == google::protobuf::FieldDescriptor::TYPE_STRING)
    {
    	size_t strlen;
    	const char *str = luaL_checklstring(L, 3, &strlen);
        reflection->SetString(message, field, str);
    }
    else if (field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES)
    {
    	size_t strlen;
    	const char *str = luaL_checklstring(L, 3, &strlen);
        reflection->SetString(message, field, str);
    }
    else if(field->type() == google::protobuf::FieldDescriptor::TYPE_INT32)
    {
        int val = static_cast<int>(luaL_checkinteger(L, 3));
        reflection->SetInt32(message, field, val);
    }
    else if(field->type() == google::protobuf::FieldDescriptor::TYPE_INT64)
    {
        long val = static_cast<long>(luaL_checknumber(L, 3));
        reflection->SetInt64(message, field, val);
    }
    else if(field->type() == google::protobuf::FieldDescriptor::TYPE_UINT32)
    {
        unsigned int val = static_cast<unsigned int>(luaL_checkinteger(L, 3));
        reflection->SetUInt32(message, field, val);
    }
    else if(field->type() == google::protobuf::FieldDescriptor::TYPE_UINT64)
    {
        unsigned long val = static_cast<unsigned long>(luaL_checknumber(L, 3));
        reflection->SetUInt64(message, field, val);
    }
    else if(field->type() == google::protobuf::FieldDescriptor::TYPE_FLOAT)
    {
        float val = static_cast<float>(luaL_checknumber(L, 3));
        reflection->SetFloat(message, field, val);
    }
    else if(field->type() == google::protobuf::FieldDescriptor::TYPE_DOUBLE)
    {
        double val = static_cast<double>(luaL_checknumber(L, 3));
        reflection->SetDouble(message, field, val);
    }
    else if(field->type() == google::protobuf::FieldDescriptor::TYPE_BOOL)
    {
        int val = static_cast<int>(luaL_checkinteger(L, 3));
        reflection->SetBool(message, field, val);
    }
    else
    {
    	luaL_argerror(L, 2, "pb_set field_name type error");
    }
    return 0;
}

static int pb_parseFromString(lua_State* L)
{
	lua_pbmsg* luamsg = (lua_pbmsg*)luaL_checkudata(L, 1, PB_MESSAGE_META);
    google::protobuf::Message *message = luamsg->msg;

    luaL_checktype(L, 2, LUA_TSTRING);

    size_t bin_len;
    const char* bin = static_cast<const char*>(	luaL_checklstring(L, 2, &bin_len));
    message->ParseFromArray(bin, bin_len);
    return 0;
}

static int pb_serializeToString(lua_State* L)
{
	lua_pbmsg* luamsg = (lua_pbmsg*)luaL_checkudata(L, 1, PB_MESSAGE_META);
    google::protobuf::Message *message = luamsg->msg;

    std::string msg;
    message->SerializeToString(&msg);
    lua_pushlstring(L, msg.c_str(), msg.length());
	return 1;
}

static int pb_unpack(lua_State* L)
{
	size_t bufLen = 0;
	char* buf =(char*)lua_touserdata(L, 1);
	bufLen = luaL_checknumber(L, 2);
	int nameLen = 0;
	memcpy(&nameLen, buf, sizeof(int));
	std::string typeName(buf+sizeof(int), nameLen);
	google::protobuf::Message *message = sProtoImporter.createDynamicMessage(typeName);
	if (!message)
	{
		fprintf(stderr, "pb_unpack error, result is typename(%s) not found!\n", typeName.c_str());
		return 0;
	}

    message->ParseFromArray(buf+sizeof(int)+nameLen, bufLen - sizeof(int) - nameLen);
	lua_pushlstring(L, typeName.c_str(), nameLen);
	push_message(L, message, true);
	return 2;
}

static int pb_pack(lua_State* L) 
{
	lua_pbmsg* luamsg = (lua_pbmsg*)luaL_checkudata(L, 1, PB_MESSAGE_META);
    google::protobuf::Message *message = luamsg->msg;

	const std::string& typeName = message->GetTypeName();
	int nameLen = typeName.size() + 1;
	int bytes = message->ByteSize();
	int len = nameLen + sizeof(int) + bytes;
	char* buf = (char*)lua_alloc(NULL, NULL, sizeof(char), len + sizeof(int));	
	// 总长度
	memcpy(buf, &len, sizeof(int));
	// pb消息名长度
	memcpy(buf + sizeof(int), &nameLen, sizeof(int));	
	// pb消息名
	memcpy(buf + 2 * sizeof(int), typeName.c_str(), nameLen + 1);
	// pb 消息内容
	message->SerializeWithCachedSizesToArray((google::protobuf::uint8*)(buf + 2*sizeof(int) + nameLen));

	lua_pushlstring(L, buf, len+sizeof(int)+1);
	lua_pushinteger(L, len+sizeof(int));
	return 2;		
}

static int pb_get_descriptor(lua_State* L) 
{
	const char* msg_name = luaL_checkstring(L, 1);	
	const google::protobuf::Descriptor* descriptor = sProtoImporter.getDescriptor(msg_name);
	if (descriptor == NULL) 
	{
		luaL_error(L, "can not find msg %s", msg_name);
	}

	lua_newtable(L);
	int i;
	for (i = 0; i < descriptor->field_count(); i++) 
	{
		const google::protobuf::FieldDescriptor* field = descriptor->field(i);
		lua_pushstring(L, field->name().data());

		lua_newtable(L);
		lua_pushstring(L, "type");
		if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_INT32 ||
			field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_UINT32 ) 
		{
			lua_pushstring(L, "int");	
		} 
		else if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_STRING) 
		{
			lua_pushstring(L, "string");
		} 
		else if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) 
		{
			lua_pushstring(L, "message");
		} 
		else if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_FLOAT) 
		{
			lua_pushstring(L, "float");
		} 
		else 
		{
			lua_pushstring(L, "unknown");
		}
		lua_settable(L, -3);

		lua_pushstring(L, "number");
		lua_pushnumber(L, field->number());
		lua_settable(L, -3);

		if (field->has_default_value()) 
		{
			lua_pushstring(L, "default");
			if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_INT32) 
			{
				lua_pushinteger(L, field->default_value_int32());
			} 
			else if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_UINT32)
			{
				lua_pushinteger(L, field->default_value_uint32());
			}
			else if (field->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_STRING) 
			{
				lua_pushstring(L, field->default_value_string().data());
			}
			else
			{
				lua_pushnil(L);
			}
			lua_settable(L, -3);
		}

		lua_settable(L, -3);
	}
	return 1;
}

static void _decode(lua_State* L, const google::protobuf::Message* message, int deep) 
{
	lua_newtable(L);

	const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
	const google::protobuf::Reflection* reflection = message->GetReflection();
	
	int i;
	for (i=0; i < descriptor->field_count(); i++)
	{
		const google::protobuf::FieldDescriptor* field = descriptor->field(i);
		if (field->is_repeated())
		{
			lua_pushstring(L, field->name().data());

			lua_newtable(L);
			int size = reflection->FieldSize(*message, field);
			int j;
			for (j = 0; j < size; j++) 
			{
				if (field->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE)
				{
					lua_pushnumber(L, j+1);				
					const google::protobuf::Message& field_message = reflection->GetRepeatedMessage(*message, field, j);
					_decode(L, &field_message, deep+1);
					lua_settable(L, -3);
				} 
				else if (field->type() == google::protobuf::FieldDescriptor::TYPE_STRING)
				{
					lua_pushnumber(L, j+1);
					lua_pushstring(L, reflection->GetRepeatedString(*message, field, j).data());
					lua_settable(L, -3);
				}
				else if (field->type() == google::protobuf::FieldDescriptor::TYPE_INT32)
				{
					lua_pushnumber(L, j+1);
					lua_pushnumber(L, reflection->GetRepeatedInt32(*message, field, j));
					lua_settable(L, -3);
				}
			}
		} 
		else if (reflection->HasField(*message, field)) 
		{
			if (field->type() == google::protobuf::FieldDescriptor::TYPE_INT32)
			{
				lua_pushstring(L, field->name().data());
				lua_pushnumber(L, reflection->GetInt32(*message, field));
				lua_settable(L, -3);
			}
			else if (field->type() == google::protobuf::FieldDescriptor::TYPE_UINT32)
			{
				lua_pushstring(L, field->name().data());
				lua_pushnumber(L, reflection->GetUInt32(*message, field));
				lua_settable(L, -3);
			}
			else if (field->type() == google::protobuf::FieldDescriptor::TYPE_FLOAT)
			{
				lua_pushstring(L, field->name().data());
				lua_pushnumber(L, reflection->GetFloat(*message, field));
				lua_settable(L, -3);
			}
			else if (field->type() == google::protobuf::FieldDescriptor::TYPE_DOUBLE)
			{
				lua_pushstring(L, field->name().data());
				lua_pushnumber(L, reflection->GetDouble(*message, field));
				lua_settable(L, -3);
			}
			else if (field->type() == google::protobuf::FieldDescriptor::TYPE_BOOL)
			{
				lua_pushstring(L, field->name().data());
				lua_pushnumber(L, reflection->GetBool(*message, field));
				lua_settable(L, -3);
			}
			else if (field->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE)
			{
				lua_pushstring(L, field->name().data());
				const google::protobuf::Message& field_message = 
					reflection->GetMessage(*message, field, &(sProtoImporter.factory));

				_decode(L, &field_message, deep + 1);
				lua_settable(L, -3);
			}
			else if (field->type() == google::protobuf::FieldDescriptor::TYPE_STRING)
			{
				lua_pushstring(L, field->name().data());
				lua_pushstring(L, reflection->GetString(*message, field).data());
				lua_settable(L, -3);
			}
			else 
			{
				luaL_error(L, "some error");
			}
		}
	}
}

static void _encode(lua_State* L, google::protobuf::Message* message) 
{
	// table in top
	//
	const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
	const google::protobuf::Reflection* reflection = message->GetReflection();
	
	int i;
	for (i=0; i < descriptor->field_count(); i++)
	{
		const google::protobuf::FieldDescriptor* field = descriptor->field(i);
		if (field->is_repeated())
		{
			lua_pushstring(L, field->name().data());
			lua_gettable(L, -2);
			if (lua_istable(L, -1))
			{
				int size = reflection->FieldSize(*message, field);
				int j;
				for (j = 0; j < size; j++) 
				{
					lua_rawgeti(L, -1, j);
					if (field->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE)
					{
    					google::protobuf::Message* field_message = reflection->MutableMessage(message, field);
						_encode(L, field_message);
					} 
					else if (field->type() == google::protobuf::FieldDescriptor::TYPE_STRING)
					{
						reflection->SetRepeatedString(message, field, i, lua_tostring(L, -1));
					}
					else if (field->type() == google::protobuf::FieldDescriptor::TYPE_INT32)
					{
						reflection->SetRepeatedInt32(message, field, i, lua_tonumber(L, -1));
					}
					lua_pop(L, 1);
				}
			} 
			else 
			{
			}
			lua_pop(L, 1);
		} 
		else if (reflection->HasField(*message, field)) 
		{
			lua_pushstring(L, field->name().data());
			lua_gettable(L, -2);
			if (field->type() == google::protobuf::FieldDescriptor::TYPE_INT32)
			{
				reflection->SetInt32(message, field, luaL_checknumber(L, -1));
			}
			else if (field->type() == google::protobuf::FieldDescriptor::TYPE_UINT32)
			{
				reflection->SetUInt32(message, field, luaL_checknumber(L, -1));
			}
			else if (field->type() == google::protobuf::FieldDescriptor::TYPE_FLOAT)
			{
				reflection->SetFloat(message, field, luaL_checknumber(L, -1));
			}
			else if (field->type() == google::protobuf::FieldDescriptor::TYPE_DOUBLE)
			{
				reflection->SetDouble(message, field, luaL_checknumber(L, -1));
			}
			else if (field->type() == google::protobuf::FieldDescriptor::TYPE_BOOL)
			{
				reflection->SetBool(message, field, luaL_checknumber(L, -1));
			}
			else if (field->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE)
			{
				_encode(L, message); 
			}
			else if (field->type() == google::protobuf::FieldDescriptor::TYPE_STRING)
			{
				reflection->SetString(message, field, lua_tostring(L, -1));
			}
			else 
			{
				luaL_error(L, "some error");
			}
			lua_pop(L, 1);
		}
	}	

}

static int pb_encode(lua_State* L) 
{	
	const char* type_name = luaL_checkstring(L, 1);
	google::protobuf::Message *message = sProtoImporter.createDynamicMessage(type_name);
	if (!message)
	{
		fprintf(stderr, "pb_encode error, result is typename(%s) not found!\n", type_name);
		return 0;
	}

	luaL_checktype(L, 2, LUA_TTABLE);

	return 0;
}

static int pb_decode(lua_State* L) 
{	
	const char* type_name = luaL_checkstring(L, 1);

	google::protobuf::Message *message = sProtoImporter.createDynamicMessage(type_name);
	if (!message)
	{
		fprintf(stderr, "pb_decode error, result is typename(%s) not found!\n", type_name);
		return 0;
	}

	int mtype = lua_type(L, 2);
	if (mtype == LUA_TSTRING) {
		size_t bin_len;
		const char* bin = static_cast<const char*>(	luaL_checklstring(L, 2, &bin_len));
		message->ParseFromArray(bin, bin_len);
	} 
	else if (mtype == LUA_TLIGHTUSERDATA) 
	{
		const char* bin = static_cast<const char*>(lua_touserdata(L, 2));
		size_t bin_len = luaL_checkinteger(L, 3);
		message->ParseFromArray(bin, bin_len);
	}
	else
	{
		luaL_error(L, "encode error");
	}

	_decode(L, message, 1);

    delete message;
	return 1;
}


static const struct luaL_Reg lib[] =
{
		{"new", pb_new},
		{"import", pb_import},
		{"tostring", pb_tostring},
		{"parseFromString", pb_parseFromString},
		{"serializeToString", pb_serializeToString},
		{"unpack", pb_unpack},
		{"pack", pb_pack},
		{"get_descriptor", pb_get_descriptor},
		{"encode", pb_encode},
		{"decode", pb_decode},
		{NULL, NULL}
};

static const struct luaL_Reg libm[] =
{
		{"__index", pb_get},
		{"__newindex", pb_set},
		{"__gc", pb_delete},
		{NULL, NULL}
};

static const struct luaL_Reg repeatedlibm[] =
{
		{"__index", pb_repeated_get},
		{"__newindex", pb_repeated_set},
		{NULL, NULL}
};

int luaopen_luapb(lua_State* L)
{
	luaL_newmetatable(L, PB_MESSAGE_META);
	lua_pushvalue(L, -1);
	lua_pushstring(L, "v");
	lua_setfield(L, -2, "__mode");
	luaL_setfuncs(L, libm, 0);
	lua_setmetatable(L, -2);

	luaL_newmetatable(L, PB_REPEATED_MESSAGE_META);
	luaL_setfuncs(L, repeatedlibm, 0);

	lua_newtable(L);
	luaL_setfuncs(L, lib, 0);

	lua_alloc = lua_getallocf(L, NULL);
	return 1;
}

int push_message(lua_State* L, google::protobuf::Message* message, bool del)
{
	lua_pushlightuserdata(L, message);
	lua_gettable(L, LUA_REGISTRYINDEX);
	if (lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		lua_pbmsg* tmp = static_cast<lua_pbmsg*>(lua_newuserdata(L, sizeof(lua_pbmsg)));
		tmp->msg = message;
		tmp->isDelete = del;
		luaL_getmetatable(L, PB_MESSAGE_META);
		lua_setmetatable(L, -2);

		//
		lua_pushlightuserdata(L, message);
		lua_pushvalue(L, -2);
		lua_settable(L, LUA_REGISTRYINDEX);
	}
	return 1;
}

