#!/usr/bin/env python

from __future__ import unicode_literals

'''Generate header file for nanopb from a ProtoBuf FileDescriptorSet.'''
nanopb_version = "nanopb-0.3.9"

import sys
import re
import codecs
from functools import reduce

try:
	# Add some dummy imports to keep packaging tools happy.
	import google, distutils.util  # bbfreeze seems to need these
	import pkg_resources  # pyinstaller / protobuf 2.5 seem to need these
except:
	# Don't care, we will error out later if it is actually important.
	pass

try:
	import google.protobuf.text_format as text_format
	import google.protobuf.descriptor_pb2 as descriptor
except:
	sys.stderr.write('''
		 *************************************************************
		 *** Could not import the Google protobuf Python libraries ***
		 *** Try installing package 'python-protobuf' or similar.  ***
		 *************************************************************
	''' + '\n')
	raise

try:
	import proto.nanopb_pb2 as nanopb_pb2
	import proto.plugin_pb2 as plugin_pb2
except TypeError:
	sys.stderr.write('''
		 ****************************************************************************
		 *** Got TypeError when importing the protocol definitions for generator. ***
		 *** This usually means that the protoc in your path doesn't match the    ***
		 *** Python protobuf library version.                                     ***
		 ***                                                                      ***
		 *** Please check the output of the following commands:                   ***
		 *** which protoc                                                         ***
		 *** protoc --version                                                     ***
		 *** python -c 'import google.protobuf; print(google.protobuf.__file__)'  ***
		 ****************************************************************************
	''' + '\n')
	raise
except:
	sys.stderr.write('''
		 ********************************************************************
		 *** Failed to import the protocol definitions for generator.     ***
		 *** You have to run 'make' in the nanopb/generator/proto folder. ***
		 ********************************************************************
	''' + '\n')
	raise

# change-begin
enumName2Value = {}  #enum strFullName 2 iValue
allMessage = {}
defaultUseStl = True
nfshmHeaderFileMap = {}
nfshmHeaderFileList = []
# change-end

# ---------------------------------------------------------------------------
#                     Generation of single fields
# ---------------------------------------------------------------------------

import time
import os.path

# Values are tuple (c type, pb type, encoded size, int_size_allowed)
FieldD = descriptor.FieldDescriptorProto
datatypes = {
	FieldD.TYPE_BOOL: ('bool', 'BOOL', 1, False),
	FieldD.TYPE_DOUBLE: ('double', 'DOUBLE', 8, False),
	FieldD.TYPE_FIXED32: ('uint32_t', 'FIXED32', 4, False),
	FieldD.TYPE_FIXED64: ('uint64_t', 'FIXED64', 8, False),
	FieldD.TYPE_FLOAT: ('float', 'FLOAT', 4, False),
	FieldD.TYPE_INT32: ('int32_t', 'INT32', 10, True),
	FieldD.TYPE_INT64: ('int64_t', 'INT64', 10, True),
	FieldD.TYPE_SFIXED32: ('int32_t', 'SFIXED32', 4, False),
	FieldD.TYPE_SFIXED64: ('int64_t', 'SFIXED64', 8, False),
	FieldD.TYPE_SINT32: ('int32_t', 'SINT32', 5, True),
	FieldD.TYPE_SINT64: ('int64_t', 'SINT64', 10, True),
	FieldD.TYPE_UINT32: ('uint32_t', 'UINT32', 5, True),
	FieldD.TYPE_UINT64: ('uint64_t', 'UINT64', 10, True)
}

# Integer size overrides (from .proto settings)
intsizes = {
	nanopb_pb2.IS_8: 'int8_t',
	nanopb_pb2.IS_16: 'int16_t',
	nanopb_pb2.IS_32: 'int32_t',
	nanopb_pb2.IS_64: 'int64_t',
}

# String types (for python 2 / python 3 compatibility)
try:
	strtypes = (unicode, str)
except NameError:
	strtypes = (str,)


class Names:
	'''Keeps a set of nested names and formats them to C identifier.'''

	def __init__(self, parts=()):
		if isinstance(parts, Names):
			parts = parts.parts
		self.parts = tuple(parts)

	def __str__(self):
		#return '_'.join(self.parts)
		i = len(self.parts) - 1
		lastName = self.parts[i]
		if lastName == "size":
			return self.parts[i - 1] + '_' + lastName
		if lastName == "tbytes":
			return self.parts[i - 2] + '_' + self.parts[i - 1] + '_t'
		return lastName

	def __add__(self, other):
		if isinstance(other, strtypes):
			return Names(self.parts + (other,))
		elif isinstance(other, tuple):
			return Names(self.parts + other)
		else:
			raise ValueError("Name parts should be of type str")

	def __eq__(self, other):
		return isinstance(other, Names) and self.parts == other.parts

	def namespace_str(self):
		return '::'.join(self.parts)


def namespace_from_type_name(type_name):
	if type_name[0] != '.':
		raise NotImplementedError("Lookup of non-absolute type names is not supported")
	return Names(type_name[1:].split('.')).namespace_str()


def names_from_type_name(type_name):
	'''Parse Names() from FieldDescriptorProto type_name'''
	if type_name[0] != '.':
		raise NotImplementedError("Lookup of non-absolute type names is not supported")
	return Names(type_name[1:].split('.'))


def varint_max_size(max_value):
	'''Returns the maximum number of bytes a varint can take when encoded.'''
	if max_value < 0:
		max_value = 2 ** 64 - max_value
	for i in range(1, 11):
		if (max_value >> (i * 7)) == 0:
			return i
	raise ValueError("Value too large for varint: " + str(max_value))


assert varint_max_size(-1) == 10
assert varint_max_size(0) == 1
assert varint_max_size(127) == 1
assert varint_max_size(128) == 2


class EncodedSize:
	'''Class used to represent the encoded size of a field or a message.
	Consists of a combination of symbolic sizes and integer sizes.'''

	def __init__(self, value=0, symbols=[]):
		if isinstance(value, EncodedSize):
			self.value = value.value
			self.symbols = value.symbols
		elif isinstance(value, strtypes + (Names,)):
			self.symbols = [str(value)]
			self.value = 0
		else:
			self.value = value
			self.symbols = symbols

	def __add__(self, other):
		if isinstance(other, int):
			return EncodedSize(self.value + other, self.symbols)
		elif isinstance(other, strtypes + (Names,)):
			return EncodedSize(self.value, self.symbols + [str(other)])
		elif isinstance(other, EncodedSize):
			return EncodedSize(self.value + other.value, self.symbols + other.symbols)
		else:
			raise ValueError("Cannot add size: " + repr(other))

	def __mul__(self, other):
		if isinstance(other, int):
			return EncodedSize(self.value * other, [str(other) + '*' + s for s in self.symbols])
		else:
			raise ValueError("Cannot multiply size: " + repr(other))

	def __str__(self):
		if not self.symbols:
			return str(self.value)
		else:
			return '(' + str(self.value) + ' + ' + ' + '.join(self.symbols) + ')'

	def upperlimit(self):
		if not self.symbols:
			return self.value
		else:
			return 2 ** 32 - 1


class Enum:
	def __init__(self, names, desc, enum_options):
		'''desc is EnumDescriptorProto'''

		self.options = enum_options
		self.names = names + desc.name

		if enum_options.long_names:
			self.values = [(self.names + x.name, x.number) for x in desc.value]
		else:
			self.values = [(names + x.name, x.number) for x in desc.value]

		self.value_longnames = [self.names + x.name for x in desc.value]
		self.value_longnames_adress_set = {id(x) for x in self.value_longnames}
		self.packed = enum_options.packed_enum

		# change-begin max_count with enum
		for x in self.values:
			enumName2Value[str(x[0])] = x[1]

	# change-end

	def has_negative(self):
		for n, v in self.values:
			if v < 0:
				return True
		return False

	def encoded_size(self):
		return max([varint_max_size(v) for n, v in self.values])

	def __str__(self):
		result = 'typedef enum _%s {\n' % self.names
		result += ',\n'.join(["    %s = %d" % x for x in self.values])
		result += '\n}'

		if self.packed:
			result += ' pb_packed'

		result += ' %s;' % self.names

		result += '\n#define _%s_MIN %s' % (self.names, self.values[0][0])
		result += '\n#define _%s_MAX %s' % (self.names, self.values[-1][0])
		result += '\n#define _%s_ARRAYSIZE ((%s)(%s+1))' % (self.names, self.names, self.values[-1][0])
		result += '\n#define _%s_ALLSIZE (%s)' % (self.names, len(self.values))

		#        if not self.options.long_names:
		# Define the long names always so that enum value references
		# from other files work properly.
		#            for i, x in enumerate(self.values):
		#                result += '\n#define %s %s' % (self.value_longnames[i], x[0])

		if self.options.enum_to_string:
			result += '\nconst char *%s_name(%s v);\n' % (self.names, self.names)

		return result

	def enum_to_string_definition(self):
		if not self.options.enum_to_string:
			return ""

		result = 'const char *%s_name(%s v) {\n' % (self.names, self.names)
		result += '    switch (v) {\n'

		for ((enumname, _), strname) in zip(self.values, self.value_longnames):
			# Strip off the leading type name from the string value.
			# change-begin max_count with enum
			# strval = str(strname)[len(str(self.names)) + 1:]
			# result += '        case %s: return "%s";\n' % (enumname, strval)
			result += '        case %s: return "%s";\n' % (enumname, enumname)
		# change-end
		result += '    }\n'
		result += '    return "unknown";\n'
		result += '}\n'

		return result


class FieldMaxSize:
	def __init__(self, worst=0, checks=[], field_name='undefined'):
		if isinstance(worst, list):
			self.worst = max(i for i in worst if i is not None)
		else:
			self.worst = worst

		self.worst_field = field_name
		self.checks = list(checks)

	def extend(self, extend, field_name=None):
		self.worst = max(self.worst, extend.worst)

		if self.worst == extend.worst:
			self.worst_field = extend.worst_field

		self.checks.extend(extend.checks)


class Field:
	def __init__(self, struct_name, desc, field_options, is_std_stl):
		'''desc is FieldDescriptorProto'''
		self.tag = desc.number
		self.struct_name = struct_name
		self.union_name = None
		self.name = desc.name
		self.default = None
		self.max_size = None
		# change-begin max_size with enum
		self.max_size_enum = None
		# change-end
		self.max_count = None
		# change-begin max_count with enum
		self.max_count_enum = None
		# change-end
		# change-begin use_stl
		self.use_stl_enum = None
		self.use_stl = None
		self.use_stl_string = False
		self.use_stl_key = None
		self.use_stl_key_type = None
		self.is_stl_key = False
		self.is_std_stl = is_std_stl
		self.is_c_std = False
		# change-end
		self.array_decl = ""
		self.enc_size = None
		self.ctype = None
		self.fixed_count = False
		self.type_name = desc.type_name

		if field_options.type == nanopb_pb2.FT_INLINE:
			# Before nanopb-0.3.8, fixed length bytes arrays were specified
			# by setting type to FT_INLINE. But to handle pointer typed fields,
			# it makes sense to have it as a separate option.
			field_options.type = nanopb_pb2.FT_STATIC
			field_options.fixed_length = True

		# change-begin
		if field_options.HasField("is_std_stl"):
			self.is_std_stl = field_options.is_std_stl

		if field_options.HasField("is_c_std"):
			self.is_c_std = field_options.is_c_std

		# change-begin
		if defaultUseStl:
			if desc.label == FieldD.LABEL_REPEATED:
				if not field_options.HasField("max_count") and not field_options.HasField("max_count_enum"):
					self.max_count_enum = "MAX_REPEATED_DEFAULT_COUNT"
					self.max_count = nanopb_pb2.MAX_REPEATED_DEFAULT_COUNT

				if not field_options.HasField("use_stl_enum") and not field_options.HasField("use_stl") and not self.is_c_std:
					self.use_stl_enum = "STL_VECTOR"
					self.use_stl = nanopb_pb2.STL_VECTOR

			if desc.type == FieldD.TYPE_STRING or desc.type == FieldD.TYPE_BYTES:
				if not field_options.HasField("max_size") and not field_options.HasField("max_size_enum"):
					self.max_size_enum = "MAX_STRING_LEN_DEFAULT"
					self.max_size = nanopb_pb2.MAX_STRING_LEN_DEFAULT

				if not field_options.HasField("use_stl_string") and not self.is_c_std:
					self.use_stl_string = True

		if self.is_std_stl:
			if desc.label == FieldD.LABEL_REPEATED:
				if not field_options.HasField("max_count") and not field_options.HasField("max_count_enum"):
					self.max_count_enum = "MAX_REPEATED_DEFAULT_COUNT"
					self.max_count = nanopb_pb2.MAX_REPEATED_DEFAULT_COUNT

			if desc.type == FieldD.TYPE_STRING or desc.type == FieldD.TYPE_BYTES:
				if not field_options.HasField("max_size") and not field_options.HasField("max_size_enum"):
					self.max_size_enum = "MAX_STRING_LEN_DEFAULT"
					self.max_size = nanopb_pb2.MAX_STRING_LEN_DEFAULT
		# change-end

		# Parse field options
		if field_options.HasField("max_size"):
			self.max_size = field_options.max_size

		# change-begin max_size with enum
		if field_options.HasField("max_size_enum"):
			self.max_size_enum = field_options.max_size_enum
			self.max_size = enumName2Value[self.max_size_enum]
		# change-end

		if desc.type == FieldD.TYPE_STRING and field_options.HasField("max_length"):
			# max_length overrides max_size for strings
			self.max_size = field_options.max_length + 1

		if field_options.HasField("max_count"):
			self.max_count = field_options.max_count

		# change-begin max_count with enum
		if field_options.HasField("max_count_enum"):
			self.max_count_enum = field_options.max_count_enum
			self.max_count = enumName2Value[self.max_count_enum]
		# change-end

		# change-begin use_stl with enum
		if field_options.HasField("use_stl_enum"):
			self.use_stl_enum = field_options.use_stl_enum
			self.use_stl = enumName2Value[self.use_stl_enum]
		# change-end

		# change-begin use_stl_key
		if field_options.HasField("use_stl_key"):
			if desc.type != FieldD.TYPE_MESSAGE:
				raise Exception("Field '%s' is defined use_stl_key, but desc.type is not message" % self.name)

			self.use_stl_key = field_options.use_stl_key
		# change-end

		# change-begin use_stl_string
		if field_options.HasField("use_stl_string"):
			self.use_stl_string = field_options.use_stl_string
		# change-end

		# change-begin is_stl_key
		if field_options.HasField("is_stl_key"):
			self.is_stl_key = field_options.is_stl_key
		# change-end

		# change-begin
		if self.use_stl is not None:
			if desc.label != FieldD.LABEL_REPEATED:
				raise Exception("Field '%s' is defined use stl, but not repeated." % self.name)

			if desc.type == FieldD.TYPE_STRING:
				if not self.use_stl_string:
					raise Exception("Field '%s' is defined repeated string and use_stl, but use_stl_string is not given" % self.name)

			if (self.use_stl == nanopb_pb2.STL_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST
					or self.use_stl == nanopb_pb2.STL_MAP or self.use_stl == nanopb_pb2.STL_MULTI_MAP):
				if desc.type == FieldD.TYPE_MESSAGE:
					if self.use_stl_key is None:
						raise Exception("Field '%s' is defined as stl unordered_map or map and desc.type is message, but use_stl_key is not given." % self.name)
				else:
					raise Exception("Field '%s' is defined as stl unordered_map or map, but desc.type is not message." % self.name)

		if self.use_stl_string and desc.type != FieldD.TYPE_STRING and desc.type != FieldD.TYPE_BYTES:
			raise Exception("Field '%s' is defined as use_stl_string, but desc.type is not string." % self.name)

		if self.use_stl_key is not None:
			if desc.label != FieldD.LABEL_REPEATED:
				raise Exception("Field '%s' is defined use stl key, but not repeated." % self.name)

			if desc.type != FieldD.TYPE_MESSAGE:
				raise Exception("Field '%s' is defined use stl key, but desc.type is not message." % self.name)

			if self.use_stl is None:
				raise Exception("Field '%s' is defined use stl key, but use stl is not given." % self.name)

			if (self.use_stl != nanopb_pb2.STL_UNORDERED_MAP and self.use_stl != nanopb_pb2.STL_MULTI_UNORDERED_MAP
					and self.use_stl != nanopb_pb2.STL_UNORDERED_MAP_LIST and self.use_stl != nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST
					and self.use_stl != nanopb_pb2.STL_MAP and self.use_stl != nanopb_pb2.STL_MULTI_MAP):
				raise Exception("Field '%s' is defined use stl key, but use stl is not map." % self.name)
		# change-end

		# change-begin shm header
		if not self.is_std_stl:
			if desc.type == FieldD.TYPE_STRING or desc.type == FieldD.TYPE_BYTES:
				if self.use_stl_string:
					if "string" not in nfshmHeaderFileMap:
						nfshmHeaderFileMap["string"] = 1
						nfshmHeaderFileList.append("#include \"NFComm/NFShmStl/NFShmString.h\"")

			if desc.label == FieldD.LABEL_REPEATED:
				if self.use_stl is not None:
					if self.use_stl == nanopb_pb2.STL_VECTOR:
						if "vector" not in nfshmHeaderFileMap:
							nfshmHeaderFileMap["vector"] = 1
							nfshmHeaderFileList.append("#include \"NFComm/NFShmStl/NFShmVector.h\"")
					elif self.use_stl == nanopb_pb2.STL_LIST:
						if "list" not in nfshmHeaderFileMap:
							nfshmHeaderFileMap["list"] = 1
							nfshmHeaderFileList.append("#include \"NFComm/NFShmStl/NFShmList.h\"")
					elif self.use_stl == nanopb_pb2.STL_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP \
							or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
						if "unordered_map" not in nfshmHeaderFileMap:
							nfshmHeaderFileMap["unordered_map"] = 1
							nfshmHeaderFileList.append("#include \"NFComm/NFShmStl/NFShmHashMap.h\"")
							nfshmHeaderFileList.append("#include \"NFComm/NFShmStl/NFShmHashMultiMap.h\"")
							nfshmHeaderFileList.append("#include \"NFComm/NFShmStl/NFShmHashMapWithList.h\"")
					elif self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET \
							or self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST:
						if "unordered_set" not in nfshmHeaderFileMap:
							nfshmHeaderFileMap["unordered_set"] = 1
							nfshmHeaderFileList.append("#include \"NFComm/NFShmStl/NFShmHashSet.h\"")
							nfshmHeaderFileList.append("#include \"NFComm/NFShmStl/NFShmHashMultiSet.h\"")
							nfshmHeaderFileList.append("#include \"NFComm/NFShmStl/NFShmHashSetWithList.h\"")
		# change-end

		if desc.HasField('default_value'):
			self.default = desc.default_value

		# Check field rules, i.e. required/optional/repeated.
		can_be_static = True
		if desc.label == FieldD.LABEL_REPEATED:
			self.rules = 'REPEATED'
			if self.max_count is None:
				can_be_static = False
			else:
				if self.is_std_stl:
					self.array_decl = ""
					self.fixed_count = field_options.fixed_count
				else:
					if self.use_stl is not None:
						self.array_decl = '%d' % self.max_count
						self.fixed_count = field_options.fixed_count
						if self.max_count_enum is not None:
							self.array_decl = '%s' % self.max_count_enum
					else:
						self.array_decl = '[%d]' % self.max_count
						self.fixed_count = field_options.fixed_count
						if self.max_count_enum is not None:
							self.array_decl = '[%s]' % self.max_count_enum

		elif field_options.proto3:
			self.rules = 'SINGULAR'
		elif desc.label == FieldD.LABEL_REQUIRED:
			self.rules = 'REQUIRED'
		elif desc.label == FieldD.LABEL_OPTIONAL:
			self.rules = 'OPTIONAL'
		else:
			raise NotImplementedError(desc.label)

		# Check if the field can be implemented with static allocation
		# i.e. whether the data size is known.
		if desc.type == FieldD.TYPE_STRING and self.max_size is None:
			can_be_static = False

		if desc.type == FieldD.TYPE_BYTES and self.max_size is None:
			can_be_static = False

		# Decide how the field data will be allocated
		if field_options.type == nanopb_pb2.FT_DEFAULT:
			if can_be_static:
				field_options.type = nanopb_pb2.FT_STATIC
			else:
				field_options.type = nanopb_pb2.FT_CALLBACK

		# change-begin max_size and max_count with enum
		if field_options.type == nanopb_pb2.FT_STATIC and not can_be_static:
			raise Exception("Field '%s' is defined as static, but max_size or "
							"max_count or max_size_enum or max_count_enum is not given." % self.name)

		if field_options.fixed_count and self.max_count is None:
			raise Exception("Field '%s' is defined as fixed count, "
							"but max_count or max_count_enum is not given." % self.name)
		# change-end

		if field_options.type == nanopb_pb2.FT_STATIC:
			self.allocation = 'STATIC'
		elif field_options.type == nanopb_pb2.FT_POINTER:
			self.allocation = 'POINTER'
		elif field_options.type == nanopb_pb2.FT_CALLBACK:
			self.allocation = 'CALLBACK'
		else:
			raise NotImplementedError(field_options.type)

		# Decide the C data type to use in the struct.
		if desc.type in datatypes:
			self.ctype, self.pbtype, self.enc_size, isa = datatypes[desc.type]

			# Override the field size if user wants to use smaller integers
			if isa and field_options.int_size != nanopb_pb2.IS_DEFAULT:
				self.ctype = intsizes[field_options.int_size]
				if desc.type == FieldD.TYPE_UINT32 or desc.type == FieldD.TYPE_UINT64:
					self.ctype = 'u' + self.ctype;
		elif desc.type == FieldD.TYPE_ENUM:
			self.pbtype = 'ENUM'
			self.ctype = names_from_type_name(desc.type_name)
			if self.default is not None:
				self.default = self.ctype + self.default
			self.enc_size = None  # Needs to be filled in when enum values are known
		elif desc.type == FieldD.TYPE_STRING:
			self.pbtype = 'STRING'
			if self.is_std_stl:
				self.ctype = 'std::string'
				self.enc_size = varint_max_size(self.max_size) + self.max_size
			else:
				if self.use_stl_string:
					self.ctype = 'NFShmString'
				else:
					self.ctype = 'char'
				if self.allocation == 'STATIC':
					if self.use_stl_string:
						self.ctype = 'NFShmString'
						if self.max_size_enum is not None:
							self.ctype += '<%s>' % self.max_size_enum;
						else:
							self.ctype += '<%d>' % self.max_size;
					else:
						if self.use_stl is not None:
							raise Exception("Field '%s' is defined use stl, repeated string"
											"but use_stl_string is False or no given." % self.name)

						self.ctype = 'char'
						if self.max_size_enum is not None:
							self.array_decl += '[%s]' % self.max_size_enum
						else:
							self.array_decl += '[%d]' % self.max_size

					self.enc_size = varint_max_size(self.max_size) + self.max_size
		elif desc.type == FieldD.TYPE_BYTES:
			if self.is_std_stl:
				self.pbtype = 'BYTES'
				self.ctype = 'std::string'
				self.enc_size = varint_max_size(self.max_size) + self.max_size
			else:
				if self.use_stl_string:
					self.pbtype = 'BYTES'
					self.ctype = 'NFShmString'
					if self.max_size_enum is not None:
						self.ctype += '<%s>' % self.max_size_enum;
					else:
						self.ctype += '<%d>' % self.max_size;

					self.enc_size = varint_max_size(self.max_size) + self.max_size
				else:
					if field_options.fixed_length:
						self.pbtype = 'FIXED_LENGTH_BYTES'
						# change-begin max_size with enum
						if self.max_size is None:
							raise Exception("Field '%s' is defined as fixed length, "
											"but max_size or max_size_enum is not given." % self.name)
						# change-end

						self.enc_size = varint_max_size(self.max_size) + self.max_size
						self.ctype = 'pb_byte_t'
						# change-begin max_size with enum
						if self.max_size_enum is not None:
							self.array_decl += '[%s]' % self.max_size_enum
						else:
							self.array_decl += '[%d]' % self.max_size
					# change-end
					else:
						self.pbtype = 'BYTES'
						self.ctype = 'pb_bytes_array_t'
						if self.allocation == 'STATIC':
							self.ctype = self.struct_name + self.name + 'tbytes'
							self.enc_size = varint_max_size(self.max_size) + self.max_size
		elif desc.type == FieldD.TYPE_MESSAGE:
			self.pbtype = 'MESSAGE'
			self.ctype = self.submsgname = names_from_type_name(desc.type_name)
			self.enc_size = None  # Needs to be filled in after the message type is available
		else:
			raise NotImplementedError(desc.type)

	def __lt__(self, other):
		return self.tag < other.tag

	def __str__(self):
		# change-begin max_count with enum
		if self.use_stl_key is not None:
			temp_msg_name = '%s' % names_from_type_name(self.type_name)
			temp_msg = allMessage[temp_msg_name]
			if temp_msg is None:
				raise Exception("Field '%s' is defined use_stl_key, but can't the message" % self.name)
			else:
				temp_field = temp_msg.fields_map[self.use_stl_key]
				if temp_field is None:
					raise Exception("Field '%s' is defined use_stl_key, but use_stl_key(%s) is not the fields of the message" % self.name, self.use_stl_key)
				if temp_field.rules == 'REPEATED':
					raise Exception("Field '%s' is defined use_stl_key, but use_stl_key(%s) is  the repeated fields of the message" % self.name, self.use_stl_key)

				self.use_stl_key_type = '%s' % temp_field.ctype
		# change-end

		result = ''
		if self.allocation == 'POINTER':
			if self.rules == 'REPEATED':
				result += '    pb_size_t ' + self.name + '_count;\n'

			if self.pbtype == 'MESSAGE':
				# Use struct definition, so recursive submessages are possible
				result += '    struct _%s *%s;' % (self.ctype, self.name)
			elif self.pbtype == 'FIXED_LENGTH_BYTES':
				# Pointer to fixed size array
				result += '    %s (*%s)%s;' % (self.ctype, self.name, self.array_decl)
			elif self.rules == 'REPEATED' and self.pbtype in ['STRING', 'BYTES']:
				# String/bytes arrays need to be defined as pointers to pointers
				result += '    %s **%s;' % (self.ctype, self.name)
			else:
				result += '    %s *%s;' % (self.ctype, self.name)
		elif self.allocation == 'CALLBACK':
			result += '    pb_callback_t %s;' % self.name
		else:
			if self.rules == 'OPTIONAL' and self.allocation == 'STATIC':
				if not defaultUseStl:
					result += '    bool has_' + self.name + ';\n'
			elif (self.rules == 'REPEATED' and
				  self.allocation == 'STATIC' and
				  not self.fixed_count):
				if self.use_stl is None and not self.is_std_stl:
					result += '    pb_size_t ' + self.name + '_count;\n'

			if self.is_std_stl:
				if self.rules == 'REPEATED':
					if self.use_stl is not None:
						if self.use_stl == nanopb_pb2.STL_VECTOR:
							result += '    std::vector<%s> %s;' % (self.ctype, self.name)
						elif self.use_stl == nanopb_pb2.STL_LIST:
							result += '    std::list<%s> %s;' % (self.ctype, self.name)
						elif self.use_stl == nanopb_pb2.STL_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST:
							result += '    std::unordered_map<%s, %s> %s;' % (self.use_stl_key_type, self.ctype, self.name)
						elif self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
							result += '    std::unordered_multimap<%s, %s> %s;' % (self.use_stl_key_type, self.ctype, self.name)
						elif self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST:
							result += '    std::unordered_set<%s> %s;' % (self.ctype, self.name)
						elif self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST:
							result += '    std::unordered_multiset<%s> %s;' % (self.ctype, self.name)
					else:
						result += '    std::vector<%s> %s%s;' % (self.ctype, self.name, self.array_decl)
				else:
					result += '    %s %s%s;' % (self.ctype, self.name, self.array_decl)
			else:
				if self.use_stl_string:
					if self.use_stl is not None:
						if self.use_stl == nanopb_pb2.STL_VECTOR:
							result += '    NFShmVector<%s,%s> %s;' % (self.ctype, self.array_decl, self.name)
						elif self.use_stl == nanopb_pb2.STL_LIST:
							result += '    NFShmList<%s,%s> %s;' % (self.ctype, self.array_decl, self.name)
						elif self.use_stl == nanopb_pb2.STL_UNORDERED_MAP:
							result += '    NFShmHashMap<%s, %s,%s> %s;' % (self.use_stl_key_type, self.ctype, self.array_decl, self.name)
						elif self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST:
							result += '    NFShmHashMapWithList<%s, %s,%s> %s;' % (self.use_stl_key_type, self.ctype, self.array_decl, self.name)
						elif self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP:
							result += '    NFShmHashMultiMap<%s, %s,%s> %s;' % (self.use_stl_key_type, self.ctype, self.array_decl, self.name)
						elif self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
							result += '    NFShmHashMultiMapWithList<%s, %s,%s> %s;' % (self.use_stl_key_type, self.ctype, self.array_decl, self.name)
						elif self.use_stl == nanopb_pb2.STL_UNORDERED_SET:
							result += '    NFShmHashSet<%s,%s> %s;' % (self.ctype, self.array_decl, self.name)
						elif self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST:
							result += '    NFShmHashSetWithList<%s,%s> %s;' % (self.ctype, self.array_decl, self.name)
						elif self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET:
							result += '    NFShmHashMultiSet<%s,%s> %s;' % (self.ctype, self.array_decl, self.name)
						elif self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST:
							result += '    NFShmHashMultiSetWithList<%s,%s> %s;' % (self.ctype, self.array_decl, self.name)
					else:
						result += '    %s %s%s;' % (self.ctype, self.name, self.array_decl)
				else:
					if self.use_stl is not None:
						if self.use_stl == nanopb_pb2.STL_VECTOR:
							result += '    NFShmVector<%s,%s> %s;' % (self.ctype, self.array_decl, self.name)
						elif self.use_stl == nanopb_pb2.STL_LIST:
							result += '    NFShmList<%s,%s> %s;' % (self.ctype, self.array_decl, self.name)
						elif self.use_stl == nanopb_pb2.STL_UNORDERED_MAP:
							result += '    NFShmHashMap<%s, %s,%s> %s;' % (self.use_stl_key_type, self.ctype, self.array_decl, self.name)
						elif self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP:
							result += '    NFShmHashMultiMap<%s, %s,%s> %s;' % (self.use_stl_key_type, self.ctype, self.array_decl, self.name)
						elif self.use_stl == nanopb_pb2.STL_UNORDERED_SET:
							result += '    NFShmHashSet<%s,%s> %s;' % (self.ctype, self.array_decl, self.name)
						elif self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET:
							result += '    NFShmHashMultiSet<%s,%s> %s;' % (self.ctype, self.array_decl, self.name)
						elif self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST:
							result += '    NFShmHashMapWithList<%s, %s,%s> %s;' % (self.use_stl_key_type, self.ctype, self.array_decl, self.name)
						elif self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
							result += '    NFShmHashMultiMapWithList<%s, %s,%s> %s;' % (self.use_stl_key_type, self.ctype, self.array_decl, self.name)
						elif self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST:
							result += '    NFShmHashSetWithList<%s,%s> %s;' % (self.ctype, self.array_decl, self.name)
						elif self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST:
							result += '    NFShmHashMultiSetWithList<%s,%s> %s;' % (self.ctype, self.array_decl, self.name)
					else:
						result += '    %s %s%s;' % (self.ctype, self.name, self.array_decl)

		return result

	def is_static(self):
		return self.allocation == 'STATIC'

	def init_str(self):
		result = ''
		if self.allocation == 'STATIC':
			if self.rules == 'REPEATED':
				if not self.fixed_count:
					if self.use_stl is not None and not self.is_std_stl:
						result += '    %s.Init();\n' % self.name
					elif self.is_std_stl:
						result += ''
					else:
						result += '    %s_count = 0;\n' % self.name
				else:
					if self.use_stl is None and not self.is_std_stl:
						result += '    memset(%s, 0, sizeof(%s));\n' % (self.name, self.name)
			else:
				if self.pbtype == 'STRING':
					if self.use_stl_string and not self.is_std_stl:
						result += '    %s.Init();\n' % self.name
					elif self.is_std_stl:
						result += ''
					else:
						result += '    %s[0] = \'\\0\';\n' % self.name
				elif self.pbtype == 'BYTES':
					if self.use_stl_string and not self.is_std_stl:
						result += '    %s.Init();\n' % self.name
					elif self.is_std_stl:
						result += ''
					else:
						result += '    %s.size = 0;\n' % self.name
				elif self.pbtype == 'FIXED_LENGTH_BYTES':
					if self.use_stl_string and not self.is_std_stl:
						result += '    %s.Init();\n' % self.name
					elif self.is_std_stl:
						result += ''
					else:
						result += '    memset(%s, 0, sizeof(%s));\n' % (self.name, self.name)
				elif self.pbtype == 'MESSAGE':
					result += '    %s.Init();\n' % self.name
				elif self.pbtype in ('ENUM', 'UENUM'):
					result = '    %s = _%s_MIN;\n' % (self.name, self.ctype)
				else:
					result += '    %s = 0;\n' % self.name
		else:
			result += '    assert(false); //%s is not a POD type\n' % self.name
		return result

	def createinit_str(self):
		result = ''
		if self.allocation == 'STATIC':
			if self.rules == 'REPEATED':
				if not self.fixed_count:
					if self.use_stl is None and not self.is_std_stl:
						result += '    %s_count = 0;\n' % self.name
						result += '    memset(&%s, 0, sizeof(%s));\n' % (self.name, self.name)
				else:
					if self.use_stl is not None or self.is_std_stl:
						if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST:
							if self.max_count_enum is not None:
								result += '    %s.resize(%s);\n' % (self.name, self.max_count_enum)
							else:
								result += '    %s.resize(%s);\n' % (self.name, self.max_count)
					else:
						result += '    memset(%s, 0, sizeof(%s));\n' % (self.name, self.name)

			else:
				if self.pbtype == 'STRING':
					if not self.use_stl_string and not self.is_std_stl:
						result += '    %s[0] = \'\\0\';\n' % self.name
				elif self.pbtype == 'BYTES':
					if not self.use_stl_string and not self.is_std_stl:
						result += '    %s.size = 0;\n' % self.name
				elif self.pbtype == 'FIXED_LENGTH_BYTES':
					if not self.use_stl_string and not self.is_std_stl:
						result += '    memset(%s, 0, sizeof(%s));\n' % (self.name, self.name)
				elif self.pbtype == 'MESSAGE':
					result += ''
				elif self.pbtype in ('ENUM', 'UENUM'):
					result = '    %s = _%s_MIN;\n' % (self.name, self.ctype)
				else:
					result += '    %s = 0;\n' % self.name
		return result

	def resumeinit_str(self):
		return ""

	def get_which_declaration_str(self):
		return ''

	def get_which_definition_str(self):
		return ''

	def copy_data_str(self):
		result = ''
		if self.allocation == 'STATIC':
			if self.rules == 'REPEATED':
				if self.is_std_stl:
					result += '    %s.clear();\n' % self.name
					result += '    %s = stArgsData.%s;\n' % (self.name, self.name)
				else:
					if self.use_stl is not None:
						if self.pbtype == 'STRING' or self.pbtype == 'BYTES' or self.pbtype == 'FIXED_LENGTH_BYTES':
							result += '    %s = stArgsData.%s;\n' % (self.name, self.name)
						else:
							result += '    %s = stArgsData.%s;\n' % (self.name, self.name)
					else:
						if not self.fixed_count:
							result += '    %s_count = stArgsData.%s_count;\n' % (self.name, self.name)
							result += '    for (pb_size_t i = 0; i < stArgsData.%s_count; ++i)\n' % self.name
						else:
							result += '    for (pb_size_t i = 0; i < %u; ++i)\n' % self.max_count
						result += '    {\n'
						if self.pbtype == 'STRING' or self.pbtype == 'BYTES' or self.pbtype == 'FIXED_LENGTH_BYTES':
							if self.use_stl_string:
								result += '        %s[i] = stArgsData.%s[i];\n' % (self.name, self.name)
							else:
								if self.pbtype == 'STRING':
									if self.max_size_enum is not None:
										result += '        snprintf(%s[i], %s, "%%s", stArgsData.%s[i]);\n' % (self.name, self.max_size_enum, self.name)
									else:
										result += '        snprintf(%s[i], %u, "%%s", stArgsData.%s[i]);\n' % (self.name, self.max_size, self.name)
								elif self.pbtype == 'BYTES':
									result += '        memcpy(%s[i].bytes, stArgsData.%s[i].bytes, stArgsData.%s[i].size);\n' % (self.name, self.name, self.name)
									result += '        %s[i].size = stArgsData.%s[i].size;\n' % (self.name, self.name)
								else:
									if self.max_size_enum is not None:
										result += '        memcpy(%s[i], stArgsData.%s[i], %s);\n' % (self.name, self.name, self.max_size_enum)
									else:
										result += '        memcpy(%s[i], stArgsData.%s[i], %u);\n' % (self.name, self.name, self.max_size)
						else:
							result += '        %s[i] = stArgsData.%s[i];\n' % (self.name, self.name)
						result += '    }\n'
			else:
				if self.is_std_stl:
					result += '    %s = stArgsData.%s;\n' % (self.name, self.name)
				else:
					if self.pbtype == 'STRING' or self.pbtype == 'BYTES' or self.pbtype == 'FIXED_LENGTH_BYTES':
						if self.use_stl_string:
							result += '    %s = stArgsData.%s;\n' % (self.name, self.name)
						else:
							if self.pbtype == 'STRING':
								if self.max_size_enum is not None:
									result += '    snprintf(%s, %s, "%%s", stArgsData.%s);\n' % (self.name, self.max_size_enum, self.name)
								else:
									result += '    snprintf(%s, %u, "%%s", stArgsData.%s);\n' % (self.name, self.max_size, self.name)
							elif self.pbtype == 'BYTES':
								result += '    memcpy(%s.bytes, stArgsData.%s.bytes, stArgsData.%s.size);\n' % (self.name, self.name, self.name)
								result += '    %s.size = stArgsData.%s.size;\n' % (self.name, self.name)
							else:
								if self.max_size_enum is not None:
									result += '    memcpy(%s, stArgsData.%s, %s);\n' % (self.name, self.name, self.max_size_enum)
								else:
									result += '    memcpy(%s, stArgsData.%s, %u);\n' % (self.name, self.name, self.max_size)
					else:
						result += '    %s = stArgsData.%s;\n' % (self.name, self.name)
		else:
			result += '    assert(false); //%s is not a POD type\n' % self.name
		return result

	def from_pb_str(self):
		result = ''
		if self.allocation == 'STATIC':
			if self.rules == 'REPEATED':
				if self.is_std_stl:
					result += '    %s.clear();\n' % self.name
					result += '    for (int i = 0; i < cc.%s_size(); ++i)\n' % self.name.lower()
					result += '    {\n'
					if self.pbtype == 'STRING' or self.pbtype == 'BYTES' or self.pbtype == 'FIXED_LENGTH_BYTES':
						if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST:
							result += '        %s.push_back(cc.%s(i));\n' % (self.name, self.name.lower())
						elif self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET:
							result += '        %s.insert(cc.%s(i));\n' % (self.name, self.name.lower())
					elif self.pbtype == 'MESSAGE':
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST:
								result += '        %s temp;\n' % self.ctype
								result += '        if (!temp.FromPb(cc.%s(i)))\n' % self.name.lower()
								result += '        {\n'
								result += '            if (NULL != g_nanopb_frompb_log_handle)\n'
								result += '                g_nanopb_frompb_log_handle("FromPb Failed, struct:%s, field:%s, cur count:%%d", cc.%s_size());\n' % (self.struct_name, self.name, self.name.lower())
								result += '            return false;\n'
								result += '        }\n'
								result += '        %s.push_back(temp);\n' % self.name
							elif self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET:
								result += '        %s temp;\n' % self.ctype
								result += '        if (!temp.FromPb(cc.%s(i)))\n' % self.name.lower()
								result += '        {\n'
								result += '            if (NULL != g_nanopb_frompb_log_handle)\n'
								result += '                g_nanopb_frompb_log_handle("FromPb Failed, struct:%s, field:%s, cur count:%%d", cc.%s_size());\n' % (self.struct_name, self.name, self.name.lower())
								result += '            return false;\n'
								result += '        }\n'
								result += '        %s.insert(temp);\n' % self.name
							elif self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST:
								result += '        %s temp;\n' % self.ctype
								result += '        if (!temp.FromPb(cc.%s(i)))\n' % self.name.lower()
								result += '        {\n'
								result += '            if (NULL != g_nanopb_frompb_log_handle)\n'
								result += '                g_nanopb_frompb_log_handle("FromPb Failed, struct:%s, field:%s, cur count:%%d", cc.%s_size());\n' % (self.struct_name, self.name, self.name.lower())
								result += '            return false;\n'
								result += '        }\n'
								result += '        %s.insert(temp);\n' % self.name
							elif self.use_stl == nanopb_pb2.STL_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP:
								result += '        %s temp;\n' % self.ctype
								result += '        if (!temp.FromPb(cc.%s(i)))\n' % self.name.lower()
								result += '        {\n'
								result += '            if (NULL != g_nanopb_frompb_log_handle)\n'
								result += '                g_nanopb_frompb_log_handle("FromPb Failed, struct:%s, field:%s, cur count:%%d", cc.%s_size());\n' % (self.struct_name, self.name, self.name.lower())
								result += '            return false;\n'
								result += '        }\n'
								result += '        %s.emplace(cc.%s(i).%s(), temp);\n' % (self.name, self.name.lower(), self.use_stl_key.lower())
							elif self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
								result += '        %s temp;\n' % self.ctype
								result += '        if (!temp.FromPb(cc.%s(i)))\n' % self.name.lower()
								result += '        {\n'
								result += '            if (NULL != g_nanopb_frompb_log_handle)\n'
								result += '                g_nanopb_frompb_log_handle("FromPb Failed, struct:%s, field:%s, cur count:%%d", cc.%s_size());\n' % (self.struct_name, self.name, self.name.lower())
								result += '            return false;\n'
								result += '        }\n'
								result += '        %s.emplace(cc.%s(i).%s(), temp);\n' % (self.name, self.name.lower(), self.use_stl_key.lower())
						else:
							result += '        %s temp;\n' % self.ctype
							result += '        if (!temp.FromPb(cc.%s(i))) return false;\n' % self.name.lower()
							result += '        %s.push_back(temp);\n' % self.name
					elif self.pbtype in ('ENUM', 'UENUM'):
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST:
								result += '        %s.push_back(static_cast<%s>(cc.%s(i)));\n' % (self.name, self.ctype, self.name.lower())
							elif self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET:
								result += '        %s.insert(static_cast<%s>(cc.%s(i)));\n' % (self.name, self.ctype, self.name.lower())
						else:
							result += '        %s.push_back(static_cast<%s>(cc.%s(i)));\n' % (self.name, self.ctype, self.name.lower())
					else:
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST:
								result += '        %s.push_back(cc.%s(i));\n' % (self.name, self.name.lower())
							elif self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET:
								result += '        %s.insert(cc.%s(i));\n' % (self.name, self.name.lower())
						else:
							result += '        %s.push_back(cc.%s(i));\n' % (self.name, self.name.lower())
					result += '    }\n'
				else:
					if self.use_stl is not None:
						result += '    %s.clear();\n' % self.name
					if self.max_count_enum is not None:
						result += '    if (cc.%s_size() > %s)\n' % (self.name.lower(), self.max_count_enum)
					else:
						result += '    if (cc.%s_size() > %u)\n' % (self.name.lower(), self.max_count)
					result += '    {\n'
					result += '        if (NULL != g_nanopb_frompb_log_handle)\n'
					if self.max_count_enum is not None:
						result += '            g_nanopb_frompb_log_handle("struct:%s, field:%s, cur count:%%d, max count:%%d", cc.%s_size(), %s);\n' % (self.struct_name, self.name, self.name.lower(), self.max_count_enum)
					else:
						result += '            g_nanopb_frompb_log_handle("struct:%s, field:%s, cur count:%%d, max count:%s", cc.%s_size());\n' % (self.struct_name, self.name, self.max_count, self.name.lower())
					result += '        return false;\n'
					result += '    }\n'
					result += '    for (int i = 0; i < cc.%s_size(); ++i)\n' % self.name.lower()
					result += '    {\n'
					if self.use_stl is not None:
						result += '        if (%s.size() >= %s.max_size())\n' % (self.name, self.name)
						result += '        {\n'
						result += '            if (NULL != g_nanopb_frompb_log_handle)\n'
						if self.max_count_enum is not None:
							result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur count:%%d, max count:%%d", cc.%s_size(), %s);\n' % (self.struct_name, self.name, self.name.lower(), self.max_count_enum)
						else:
							result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur count:%%d, max count:%s", cc.%s_size());\n' % (self.struct_name, self.name, self.max_count, self.name.lower())
						result += '            return false;\n'
						result += '        }\n'
					if self.pbtype == 'STRING':
						if self.max_size_enum is not None:
							result += '        if (cc.%s(i).length() >= %s)\n' % (self.name.lower(), self.max_size_enum)
						else:
							result += '        if (cc.%s(i).length() >= %u)\n' % (self.name.lower(), self.max_size)
						result += '        {\n'
						result += '            if (NULL != g_nanopb_frompb_log_handle)\n'
						if self.max_size_enum is not None:
							result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur length:%%d, max length:%%d", cc.%s(i).length(), %s-1);\n' % (self.struct_name, self.name, self.name.lower(), self.max_size_enum)
						else:
							result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur length:%%d, max length:%s", cc.%s(i).length());\n' % (self.struct_name, self.name, self.max_size - 1, self.name.lower())
						result += '            return false;\n'
						result += '        }\n'
						if self.use_stl_string:
							if self.use_stl is not None:
								if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST:
									result += '        %s.push_back(cc.%s(i));\n' % (self.name, self.name.lower())
								elif self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET:
									result += '        %s.insert(cc.%s(i));\n' % (self.name, self.name.lower())
							else:
								result += '        %s[i] = cc.%s(i);\n' % (self.name, self.name.lower())
						else:
							if self.max_size_enum is not None:
								result += '        snprintf(%s[i], %s, "%%s", cc.%s(i).c_str());\n' % (self.name, self.max_size_enum, self.name.lower())
							else:
								result += '        snprintf(%s[i], %u, "%%s", cc.%s(i).c_str());\n' % (self.name, self.max_size, self.name.lower())
					elif self.pbtype == 'BYTES':
						if self.max_size_enum is not None:
							result += '        if (cc.%s(i).size() >= %s)\n' % (self.name.lower(), self.max_size_enum)
						else:
							result += '        if (cc.%s(i).size() >= %u)\n' % (self.name.lower(), self.max_size)
						result += '        {\n'
						result += '            if (NULL != g_nanopb_frompb_log_handle)\n'
						if self.max_size_enum is not None:
							result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur size:%%d, max size:%%d", cc.%s(i).size(), %s);\n' % (self.struct_name, self.name, self.name.lower(), self.max_size_enum)
						else:
							result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur size:%%d, max size:%s", cc.%s(i).size());\n' % (self.struct_name, self.name, self.max_size, self.name.lower())
						result += '            return false;\n'
						result += '        }\n'
						if self.use_stl_string:
							if self.use_stl is not None:
								if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST:
									result += '        %s.push_back(cc.%s(i));\n' % (self.name, self.name.lower())
								elif self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET:
									result += '        %s.insert(cc.%s(i));\n' % (self.name, self.name.lower())
							else:
								result += '        %s[i] = cc.%s(i);\n' % (self.name, self.name.lower())
						else:
							result += '        memcpy(%s[i].bytes, cc.%s(i).data(), cc.%s(i).size());\n' % (self.name, self.name.lower(), self.name.lower())
							result += '        %s[i].size = cc.%s(i).size();\n' % (self.name, self.name.lower())
					elif self.pbtype == 'FIXED_LENGTH_BYTES':
						if self.max_size_enum is not None:
							result += '        if (cc.%s(i).size() > %s)\n' % (self.name.lower(), self.max_size_enum)
						else:
							result += '        if (cc.%s(i).size() > %u)\n' % (self.name.lower(), self.max_size)
						result += '        {\n'
						result += '            if (NULL != g_nanopb_frompb_log_handle)\n'
						if self.max_size_enum is not None:
							result += '            g_nanopb_frompb_log_handle("struct:%s, field:%s, cur count:%%d, max size:%%d", cc.%s_size(), %s);\n' % (self.struct_name, self.name, self.name.lower(), self.max_size_enum)
						else:
							result += '            g_nanopb_frompb_log_handle("struct:%s, field:%s, cur count:%%d, max size:%s", cc.%s_size());\n' % (self.struct_name, self.name, self.max_size, self.name.lower())
						result += '            return false;\n'
						result += '        }\n'
						if self.use_stl_string:
							if self.use_stl is not None:
								if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST:
									result += '        %s.push_back(cc.%s(i));\n' % (self.name, self.name.lower())
								elif self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET:
									result += '        %s.insert(cc.%s(i));\n' % (self.name, self.name.lower())
							else:
								result += '        %s[i] = cc.%s(i);\n' % (self.name, self.name.lower())
						else:
							result += '        memcpy(%s[i], cc.%s(i).data(), cc.%s(i).size());\n' % (self.name, self.name.lower(), self.name.lower())
					elif self.pbtype == 'MESSAGE':
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST:
								result += '        %s.emplace_back();\n' % self.name
								result += '        if (!%s.back().FromPb(cc.%s(i)))\n' % (self.name, self.name.lower())
								result += '        {\n'
								result += '            if (NULL != g_nanopb_frompb_log_handle)\n'
								if self.max_count_enum is not None:
									result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur count:%%d, max count:%%d", cc.%s_size(), %s);\n' % (self.struct_name, self.name, self.name.lower(), self.max_count_enum)
								else:
									result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur count:%%d, max count:%s", cc.%s_size());\n' % (self.struct_name, self.name, self.max_count, self.name.lower())
								result += '            return false;\n'
								result += '        }\n'
							elif self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET:
								result += '        %s temp;\n' % self.ctype
								result += '        if (!temp.FromPb(cc.%s(i)))\n' % self.name.lower()
								result += '        {\n'
								result += '            if (NULL != g_nanopb_frompb_log_handle)\n'
								if self.max_count_enum is not None:
									result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur count:%%d, max count:%%d", cc.%s_size(), %s);\n' % (self.struct_name, self.name, self.name.lower(), self.max_count_enum)
								else:
									result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur count:%%d, max count:%s", cc.%s_size());\n' % (self.struct_name, self.name, self.max_count, self.name.lower())
								result += '            return false;\n'
								result += '        }\n'
								result += '        %s.insert(temp);\n' % self.name
							elif self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST:
								result += '        %s temp;\n' % self.ctype
								result += '        if (!temp.FromPb(cc.%s(i)))\n' % self.name.lower()
								result += '        {\n'
								result += '            if (NULL != g_nanopb_frompb_log_handle)\n'
								if self.max_count_enum is not None:
									result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur count:%%d, max count:%%d", cc.%s_size(), %s);\n' % (self.struct_name, self.name, self.name.lower(), self.max_count_enum)
								else:
									result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur count:%%d, max count:%s", cc.%s_size());\n' % (self.struct_name, self.name, self.max_count, self.name.lower())
								result += '            return false;\n'
								result += '        }\n'
								result += '        %s.insert(temp);\n' % self.name
							elif self.use_stl == nanopb_pb2.STL_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP:
								result += '        %s temp;\n' % self.ctype
								result += '        if (!temp.FromPb(cc.%s(i)))\n' % self.name.lower()
								result += '        {\n'
								result += '            if (NULL != g_nanopb_frompb_log_handle)\n'
								if self.max_count_enum is not None:
									result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur count:%%d, max count:%%d", cc.%s_size(), %s);\n' % (self.struct_name, self.name, self.name.lower(), self.max_count_enum)
								else:
									result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur count:%%d, max count:%s", cc.%s_size());\n' % (self.struct_name, self.name, self.max_count, self.name.lower())
								result += '            return false;\n'
								result += '        }\n'
								result += '        %s.emplace(cc.%s(i).%s(), temp);\n' % (self.name, self.name.lower(), self.use_stl_key.lower())
							elif self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
								result += '        %s temp;\n' % self.ctype
								result += '        if (!temp.FromPb(cc.%s(i)))\n' % self.name.lower()
								result += '        {\n'
								result += '            if (NULL != g_nanopb_frompb_log_handle)\n'
								if self.max_count_enum is not None:
									result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur count:%%d, max count:%%d", cc.%s_size(), %s);\n' % (self.struct_name, self.name, self.name.lower(), self.max_count_enum)
								else:
									result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur count:%%d, max count:%s", cc.%s_size());\n' % (self.struct_name, self.name, self.max_count, self.name.lower())
								result += '            return false;\n'
								result += '        }\n'
								result += '        %s.emplace(cc.%s(i).%s(), temp);\n' % (self.name, self.name.lower(), self.use_stl_key.lower())
						else:
							result += '        if (!%s[i].FromPb(cc.%s(i))) return false;\n' % (self.name, self.name.lower())
					elif self.pbtype in ('ENUM', 'UENUM'):
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST:
								result += '        %s.push_back(static_cast<%s>(cc.%s(i)));\n' % (self.name, self.ctype, self.name.lower())
							elif self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET:
								result += '        %s.insert(static_cast<%s>(cc.%s(i)));\n' % (self.name, self.ctype, self.name.lower())
						else:
							result += '        %s[i] = static_cast<%s>(cc.%s(i));\n' % (self.name, self.ctype, self.name.lower())
					else:
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST:
								result += '        %s.push_back(cc.%s(i));\n' % (self.name, self.name.lower())
							elif self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET:
								result += '        %s.insert(cc.%s(i));\n' % (self.name, self.name.lower())
						else:
							result += '        %s[i] = cc.%s(i);\n' % (self.name, self.name.lower())
					result += '    }\n'
					if not self.fixed_count:
						if self.use_stl is None:
							result += '    %s_count = cc.%s_size();\n' % (self.name, self.name.lower())
			else:
				if self.is_std_stl:
					if self.pbtype == 'STRING' or self.pbtype == 'BYTES' or self.pbtype == 'FIXED_LENGTH_BYTES':
						result += '    %s = cc.%s();\n' % (self.name, self.name.lower())
					elif self.pbtype == 'MESSAGE':
						result += '    if (!%s.FromPb(cc.%s())) return false;\n' % (self.name, self.name.lower())
					elif self.pbtype in ('ENUM', 'UENUM'):
						result += '    %s = static_cast<%s>(cc.%s());\n' % (self.name, self.ctype, self.name.lower())
					else:
						result += '    %s = cc.%s();\n' % (self.name, self.name.lower())
				else:
					if self.pbtype == 'STRING':
						result += '    if (cc.%s().length() >= %u)\n' % (self.name.lower(), self.max_size)
						result += '    {\n'
						result += '        if (NULL != g_nanopb_frompb_log_handle)\n'
						result += '            g_nanopb_frompb_log_handle("struct:%s, field:%s, cur length:%%d, max length:%s", cc.%s().length());\n' % (self.struct_name, self.name, self.max_size - 1, self.name.lower())
						result += '        return false;\n'
						result += '    }\n'
						if self.use_stl_string:
							result += '    %s = cc.%s();\n' % (self.name, self.name.lower())
						else:
							if self.max_size_enum is not None:
								result += '    snprintf(%s, %s, "%%s", cc.%s().c_str());\n' % (self.name, self.max_size_enum, self.name.lower())
							else:
								result += '    snprintf(%s, %u, "%%s", cc.%s().c_str());\n' % (self.name, self.max_size, self.name.lower())
					elif self.pbtype == 'BYTES':
						result += '    if (cc.%s().size() > %u)\n' % (self.name.lower(), self.max_size)
						result += '    {\n'
						result += '        if (NULL != g_nanopb_frompb_log_handle)\n'
						result += '            g_nanopb_frompb_log_handle("struct:%s, field:%s, cur size:%%d, max size:%s", cc.%s().size());\n' % (self.struct_name, self.name, self.max_size, self.name.lower())
						result += '        return false;\n'
						result += '    }\n'
						if self.use_stl_string:
							result += '    %s = cc.%s();\n' % (self.name, self.name.lower())
						else:
							result += '    memcpy(%s.bytes, cc.%s().data(), cc.%s().size());\n' % (self.name, self.name.lower(), self.name.lower())
							result += '    %s.size = cc.%s().size();\n' % (self.name, self.name.lower())
					elif self.pbtype == 'FIXED_LENGTH_BYTES':
						result += '    if (cc.%s().size() > %u)\n' % (self.name.lower(), self.max_size)
						result += '    {\n'
						result += '        if (NULL != g_nanopb_frompb_log_handle)\n'
						result += '            g_nanopb_frompb_log_handle("struct:%s, field:%s, cur size:%%d, max size:%s", cc.%s().size());\n' % (self.struct_name, self.name, self.max_size, self.name.lower())
						result += '        return false;\n'
						result += '    }\n'
						if self.use_stl_string:
							result += '    %s = cc.%s();\n' % (self.name, self.name.lower())
						else:
							result += '    memcpy(%s, cc.%s().data(), cc.%s().size());\n' % (self.name, self.name.lower(), self.name.lower())
					elif self.pbtype == 'MESSAGE':
						result += '    if (!%s.FromPb(cc.%s())) return false;\n' % (self.name, self.name.lower())
					elif self.pbtype in ('ENUM', 'UENUM'):
						result += '    %s = static_cast<%s>(cc.%s());\n' % (self.name, self.ctype, self.name.lower())
					else:
						result += '    %s = cc.%s();\n' % (self.name, self.name.lower())
		else:
			result += '    assert(false); //%s is not a POD type\n' % self.name
		return result

	def to_pb_str(self):
		result = ''
		if self.allocation == 'STATIC':
			if self.rules == 'REPEATED':
				result += '    cc->clear_%s();\n' % self.name.lower()
				if self.is_std_stl:
					if (self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST
							or self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST):
						result += '    for (auto iter = %s.list_begin(); iter != %s.list_end(); ++iter)\n' % (self.name, self.name)
						result += '    {\n'
					else:
						result += '    for (auto iter = %s.begin(); iter != %s.end(); ++iter)\n' % (self.name, self.name)
						result += '    {\n'
					if self.pbtype == 'MESSAGE':
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST or self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST:
								result += '        iter->ToPb(cc->add_%s());\n' % (self.name.lower())
							elif self.use_stl == nanopb_pb2.STL_MAP or self.use_stl == nanopb_pb2.STL_MULTI_MAP or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
								result += '        iter->second.ToPb(cc->add_%s());\n' % (self.name.lower())
						else:
							result += '        iter->ToPb(cc->add_%s());\n' % (self.name.lower())
					elif self.pbtype in ('ENUM', 'UENUM'):
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST:
								result += '        cc->add_%s(static_cast<%s>(*iter));\n' % (self.name.lower(), namespace_from_type_name(self.type_name))
							elif self.use_stl == nanopb_pb2.STL_MAP or self.use_stl == nanopb_pb2.STL_MULTI_MAP or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
								result += '        cc->add_%s(static_cast<%s>(iter->second));\n' % (self.name.lower(), namespace_from_type_name(self.type_name))
						else:
							result += '        cc->add_%s(static_cast<%s>(*iter));\n' % (self.name.lower(), namespace_from_type_name(self.type_name))
					else:
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST or self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST:
								result += '        cc->add_%s(*iter);\n' % (self.name.lower())
							elif self.use_stl == nanopb_pb2.STL_MAP or self.use_stl == nanopb_pb2.STL_MULTI_MAP or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
								result += '        cc->add_%s(iter->second);\n' % (self.name.lower())
						else:
							result += '        cc->add_%s(*iter);\n' % (self.name.lower())
					result += '    }\n'
				else:
					if not self.fixed_count:
						if self.use_stl is not None:
							if (self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST
									or self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST):
								result += '    for (auto iter = %s.list_begin(); iter != %s.list_end(); ++iter)\n' % (self.name, self.name)
								result += '    {\n'
							else:
								result += '    for (auto iter = %s.begin(); iter != %s.end(); ++iter)\n' % (self.name, self.name)
								result += '    {\n'
						else:
							result += '    for (pb_size_t i = 0; i < %s_count; ++i)\n' % self.name
							result += '    {\n'
					else:
						if self.use_stl is not None:
							if (self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST
									or self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST):
								result += '    for (auto iter = %s.list_begin(); iter != %s.list_end(); ++iter)\n' % (self.name, self.name)
								result += '    {\n'
							else:
								result += '    for (auto iter = %s.begin(); iter != %s.end(); ++iter)\n' % (self.name, self.name)
								result += '    {\n'
						else:
							result += '    for (pb_size_t i = 0; i < %u; ++i)\n' % self.max_count
							result += '    {\n'
					if self.pbtype == 'STRING':
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST or self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST:
								if self.use_stl_string is True:
									result += '        cc->add_%s(iter->data());\n' % (self.name.lower())
								else:
									result += '        cc->add_%s(*iter);\n' % (self.name.lower())
							elif self.use_stl == nanopb_pb2.STL_MAP or self.use_stl == nanopb_pb2.STL_MULTI_MAP or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
								if self.use_stl_string is True:
									result += '        cc->add_%s(iter->second.data());\n' % (self.name.lower())
								else:
									result += '        cc->add_%s(iter->second);\n' % (self.name.lower())
						else:
							if self.use_stl_string is True:
								result += '        cc->add_%s(%s[i].data());\n' % (self.name.lower(), self.name)
							else:
								result += '        cc->add_%s(%s[i]);\n' % (self.name.lower(), self.name)
					elif self.pbtype == 'BYTES':
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST or self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST:
								if self.use_stl_string is True:
									result += '        cc->add_%s(iter->data());\n' % (self.name.lower())
								else:
									result += '        cc->add_%s(*iter);\n' % (self.name.lower())
							elif self.use_stl == nanopb_pb2.STL_MAP or self.use_stl == nanopb_pb2.STL_MULTI_MAP or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
								if self.use_stl_string is True:
									result += '        cc->add_%s(iter->second.data());\n' % (self.name.lower())
								else:
									result += '        cc->add_%s(iter->second);\n' % (self.name.lower())
						else:
							if self.use_stl_string is True:
								result += '        cc->add_%s(%s[i].data(), %s[i].length());\n' % (self.name.lower(), self.name, self.name)
							else:
								result += '        cc->add_%s(%s[i].bytes, %s[i].size);\n' % (self.name.lower(), self.name, self.name)
					elif self.pbtype == 'FIXED_LENGTH_BYTES':
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST or self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST:
								if self.use_stl_string is True:
									result += '        cc->add_%s(iter->data());\n' % (self.name.lower())
								else:
									result += '        cc->add_%s(*iter);\n' % (self.name.lower())
							elif self.use_stl == nanopb_pb2.STL_MAP or self.use_stl == nanopb_pb2.STL_MULTI_MAP or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
								if self.use_stl_string is True:
									result += '        cc->add_%s(iter->second.data());\n' % (self.name.lower())
								else:
									result += '        cc->add_%s(iter->second);\n' % (self.name.lower())
						else:
							if self.use_stl_string is True:
								result += '        cc->add_%s(%s[i].data(), %s[i].length());\n' % (self.name.lower(), self.name, self.name)
							else:
								result += '        cc->add_%s(%s[i], %u);\n' % (self.name.lower(), self.name, self.max_size)
					elif self.pbtype == 'MESSAGE':
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST or self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST:
								result += '        iter->ToPb(cc->add_%s());\n' % (self.name.lower())
							elif self.use_stl == nanopb_pb2.STL_MAP or self.use_stl == nanopb_pb2.STL_MULTI_MAP or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
								result += '        iter->second.ToPb(cc->add_%s());\n' % (self.name.lower())
						else:
							result += '        %s[i].ToPb(cc->add_%s());\n' % (self.name, self.name.lower())
					elif self.pbtype in ('ENUM', 'UENUM'):
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST or self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST:
								result += '        cc->add_%s(static_cast<%s>(*iter));\n' % (self.name.lower(), namespace_from_type_name(self.type_name))
							elif self.use_stl == nanopb_pb2.STL_MAP or self.use_stl == nanopb_pb2.STL_MULTI_MAP or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
								result += '        cc->add_%s(static_cast<%s>(iter->second));\n' % (self.name.lower(), namespace_from_type_name(self.type_name))
						else:
							result += '        cc->add_%s(static_cast<%s>(%s[i]));\n' % (self.name.lower(), namespace_from_type_name(self.type_name), self.name)
					else:
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST or self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST:
								result += '        cc->add_%s(*iter);\n' % (self.name.lower())
							elif self.use_stl == nanopb_pb2.STL_MAP or self.use_stl == nanopb_pb2.STL_MULTI_MAP or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
								result += '        cc->add_%s(iter->second);\n' % (self.name.lower())
						else:
							result += '        cc->add_%s(%s[i]);\n' % (self.name.lower(), self.name)
					result += '    }\n'
			else:
				if self.is_std_stl:
					if self.pbtype == 'MESSAGE':
						result += '    %s.ToPb(cc->mutable_%s());\n' % (self.name, self.name.lower())
					elif self.pbtype in ('ENUM', 'UENUM'):
						result = '    cc->set_%s(static_cast<%s>(%s));\n' % (self.name.lower(), namespace_from_type_name(self.type_name), self.name)
					else:
						result += '    cc->set_%s(%s);\n' % (self.name.lower(), self.name)
				else:
					if self.pbtype == 'STRING':
						if self.use_stl_string:
							result += '    cc->set_%s(%s.data());\n' % (self.name.lower(), self.name)
						else:
							result += '    cc->set_%s(%s);\n' % (self.name.lower(), self.name)
					elif self.pbtype == 'BYTES':
						if self.use_stl_string:
							result += '    cc->set_%s(%s.data(), %s.length());\n' % (self.name.lower(), self.name, self.name)
						else:
							result += '    cc->set_%s(%s.bytes, %s.size);\n' % (self.name.lower(), self.name, self.name)
					elif self.pbtype == 'FIXED_LENGTH_BYTES':
						if self.use_stl_string:
							result += '    cc->set_%s(%s.data(), %s.length());\n' % (self.name.lower(), self.name, self.name)
						else:
							result += '    cc->set_%s(%s, %u);\n' % (self.name.lower(), self.name, self.max_size)
					elif self.pbtype == 'MESSAGE':
						result += '    %s.ToPb(cc->mutable_%s());\n' % (self.name, self.name.lower())
					elif self.pbtype in ('ENUM', 'UENUM'):
						result += '    cc->set_%s(static_cast<%s>(%s));\n' % (self.name.lower(), namespace_from_type_name(self.type_name), self.name)
					else:
						result += '    cc->set_%s(%s);\n' % (self.name.lower(), self.name)
		else:
			result += '    assert(false); //%s is not a POD type\n' % self.name
		return result

	def destruct_str(self):
		return ""

	def short_debug_string_str(self):
		result = ''
		if self.allocation == 'STATIC':
			if self.rules == 'REPEATED':
				if self.is_std_stl:
					result += '    ss << "%s(" << %s.size()<< "):[";\n' % (self.name, self.name)
					result += '    for (auto iter = %s.begin(); iter != %s.end(); ++iter)\n' % (self.name, self.name)
					result += '    {\n'
					if self.pbtype == 'BYTES':
						result += '        ss << "(skip print BYTES)";\n'
						result += '        break;\n'
					elif self.pbtype == 'FIXED_LENGTH_BYTES':
						result += '        ss << "(skip print FIXED_LENGTH_BYTES)";\n'
						result += '        break;\n'
					elif self.pbtype == 'MESSAGE':
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST or self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST:
								result += '        ss << iter->ShortDebugString();\n'
							elif self.use_stl == nanopb_pb2.STL_MAP or self.use_stl == nanopb_pb2.STL_MULTI_MAP or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
								result += '        ss << iter->second.ShortDebugString();\n'
						else:
							result += '        ss << iter->ShortDebugString();\n'
					elif self.pbtype == 'STRING':
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST or self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST:
								result += '        ss << "\\"" << *iter << "\\"";\n'
							elif self.use_stl == nanopb_pb2.STL_MAP or self.use_stl == nanopb_pb2.STL_MULTI_MAP or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
								result += '        ss << "\\"" << iter->second << "\\"";\n'
						else:
							result += '        ss << "\\"" << *iter << "\\"";\n'
					else:
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST or self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST:
								if self.ctype == "int8_t":
									result += '    ss << static_cast<int>(*iter);\n'
								elif self.ctype == "uint8_t":
									result += '    ss << static_cast<unsigned int>(*iter);\n'
								else:
									result += '        ss << *iter;\n'
							elif self.use_stl == nanopb_pb2.STL_MAP or self.use_stl == nanopb_pb2.STL_MULTI_MAP or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
								if self.ctype == "int8_t":
									result += '    ss << static_cast<int>(iter->second);\n'
								elif self.ctype == "uint8_t":
									result += '    ss << static_cast<unsigned int>(iter->second);\n'
								else:
									result += '        ss << *iter;\n'
						else:
							if self.ctype == "int8_t":
								result += '    ss << static_cast<int>(*iter);\n'
							elif self.ctype == "uint8_t":
								result += '    ss << static_cast<unsigned int>(*iter);\n'
							else:
								result += '        ss << *iter;\n'
					result += '    }\n'
				else:
					if not self.fixed_count:
						if self.use_stl is not None:
							result += '    ss << "%s(" << %s.size()<< "):[";\n' % (self.name, self.name)
							result += '    for (auto iter = %s.begin(); iter != %s.end(); ++iter)\n' % (self.name, self.name)
						else:
							result += '    ss << "%s(" << %s_count << "):[";\n' % (self.name, self.name)
							result += '    for (pb_size_t i = 0; i < %s_count; ++i)\n' % self.name
					else:
						if self.use_stl is not None:
							result += '    ss << "%s(" << %s.size()<< "):[";\n' % (self.name, self.name)
							result += '    for (auto iter = %s.begin(); iter != %s.end(); ++iter)\n' % (self.name, self.name)
						else:
							result += '    ss << "%s(" << %u << "):[";\n' % (self.name, self.max_count)
							result += '    for (pb_size_t i = 0; i < %u; ++i)\n' % self.max_count
					result += '    {\n'
					if self.pbtype == 'BYTES':
						result += '        ss << "(skip print BYTES)";\n'
						result += '        break;\n'
					elif self.pbtype == 'FIXED_LENGTH_BYTES':
						result += '        ss << "(skip print FIXED_LENGTH_BYTES)";\n'
						result += '        break;\n'
					elif self.pbtype == 'MESSAGE':
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST or self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST:
								result += '        ss << iter->ShortDebugString();\n'
							elif self.use_stl == nanopb_pb2.STL_MAP or self.use_stl == nanopb_pb2.STL_MULTI_MAP or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
								result += '        ss << iter->second.ShortDebugString();\n'
						else:
							result += '        ss << %s[i].ShortDebugString();\n' % self.name
					elif self.pbtype == 'STRING':
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST or self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST:
								if self.use_stl_string == True:
									result += '        ss << "\\"" << iter->data() << "\\"";\n'
								else:
									result += '        ss << "\\"" << *iter << "\\"";\n'
							elif self.use_stl == nanopb_pb2.STL_MAP or self.use_stl == nanopb_pb2.STL_MULTI_MAP or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
								if self.use_stl_string == True:
									result += '        ss << "\\"" << iter->second.data() << "\\"";\n'
								else:
									result += '        ss << "\\"" << iter->second << "\\"";\n'
						else:
							if self.use_stl_string == True:
								result += '        ss << "\\"" << %s[i].data() << "\\"";\n' % self.name
							else:
								result += '        ss << "\\"" << %s[i] << "\\"";\n' % self.name
					else:
						if self.use_stl is not None:
							if self.use_stl == nanopb_pb2.STL_VECTOR or self.use_stl == nanopb_pb2.STL_LIST or self.use_stl == nanopb_pb2.STL_UNORDERED_SET or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_SET_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_SET_LIST:
								if self.ctype == "int8_t":
									result += '    ss << static_cast<int>(*iter);\n'
								elif self.ctype == "uint8_t":
									result += '    ss << static_cast<unsigned int>(*iter);\n'
								else:
									result += '        ss << *iter;\n'
							elif self.use_stl == nanopb_pb2.STL_MAP or self.use_stl == nanopb_pb2.STL_MULTI_MAP or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP \
									or self.use_stl == nanopb_pb2.STL_UNORDERED_MAP_LIST or self.use_stl == nanopb_pb2.STL_MULTI_UNORDERED_MAP_LIST:
								if self.ctype == "int8_t":
									result += '    ss << static_cast<int>(iter->second);\n'
								elif self.ctype == "uint8_t":
									result += '    ss << static_cast<unsigned int>(iter->second);\n'
								else:
									result += '        ss << *iter;\n'
						else:
							if self.ctype == "int8_t":
								result += '    ss << static_cast<int>(%s[i]);\n' % self.name
							elif self.ctype == "uint8_t":
								result += '    ss << static_cast<unsigned int>(%s[i]);\n' % self.name
							else:
								result += '        ss << %s[i];\n' % self.name
					if not self.fixed_count:
						if self.use_stl is not None:
							result += '        ss << ", ";\n'
						else:
							result += '        if (i < (%s_count - 1)) ss << ", ";\n' % self.name
					else:
						if self.use_stl is not None:
							result += '        ss << ", ";\n'
						else:
							result += '        if (i < (%u - 1)) ss << ", ";\n' % self.max_count
					result += '    }\n'
					result += '    ss << "]"'
			else:
				result += '    ss << "%s:"' % self.name
				if self.pbtype == 'BYTES':
					result += ' << "(skip print BYTES)"'
				elif self.pbtype == 'FIXED_LENGTH_BYTES':
					result += ' << "(skip print FIXED_LENGTH_BYTES)"'
				elif self.pbtype == 'MESSAGE':
					result += ' << %s.ShortDebugString()' % self.name
				elif self.pbtype == 'STRING':
					if self.is_std_stl:
						result += ' << "\\"" << %s << "\\""' % self.name
					else:
						if self.use_stl_string:
							result += ' << "\\"" << %s.data() << "\\""' % self.name
						else:
							result += ' << "\\"" << %s << "\\""' % self.name
				else:
					if self.ctype == "int8_t":
						result += ' << static_cast<int>(%s)' % self.name
					elif self.ctype == "uint8_t":
						result += ' << static_cast<unsigned int>(%s)' % self.name
					else:
						result += ' << %s' % self.name
		else:
			result += '    assert(false); //%s is not a POD type\n' % self.name
		return result

	def types(self):
		'''Return definitions for any special types this field might need.'''
		if not self.use_stl_string and not self.is_std_stl:
			if self.pbtype == 'BYTES' and self.allocation == 'STATIC':
				result = 'typedef PB_BYTES_ARRAY_T(%d) %s;\n' % (self.max_size, self.ctype)
			else:
				result = ''
		else:
			result = ''

		return result

	def get_dependencies(self):
		'''Get list of type names used by this field.'''
		if self.allocation == 'STATIC':
			return [str(self.ctype)]
		else:
			return []

	def get_initializer(self, null_init, inner_init_only=False):
		'''Return literal expression for this field's default value.
		null_init: If True, initialize to a 0 value instead of default from .proto
		inner_init_only: If True, exclude initialization for any count/has fields
		'''

		inner_init = None
		if self.pbtype == 'MESSAGE':
			if null_init:
				inner_init = '%s_init_zero' % self.ctype
			else:
				inner_init = '%s_init_default' % self.ctype
		elif self.default is None or null_init:
			if self.pbtype == 'STRING':
				inner_init = '""'
			elif self.pbtype == 'BYTES':
				inner_init = '{0, {0}}'
			elif self.pbtype == 'FIXED_LENGTH_BYTES':
				inner_init = '{0}'
			elif self.pbtype in ('ENUM', 'UENUM'):
				inner_init = '_%s_MIN' % self.ctype
			else:
				inner_init = '0'
		else:
			if self.pbtype == 'STRING':
				data = codecs.escape_encode(self.default.encode('utf-8'))[0]
				inner_init = '"' + data.decode('ascii') + '"'
			elif self.pbtype == 'BYTES':
				data = codecs.escape_decode(self.default)[0]
				data = ["0x%02x" % c for c in bytearray(data)]
				if len(data) == 0:
					inner_init = '{0, {0}}'
				else:
					inner_init = '{%d, {%s}}' % (len(data), ','.join(data))
			elif self.pbtype == 'FIXED_LENGTH_BYTES':
				data = codecs.escape_decode(self.default)[0]
				data = ["0x%02x" % c for c in bytearray(data)]
				if len(data) == 0:
					inner_init = '{0}'
				else:
					inner_init = '{%s}' % ','.join(data)
			elif self.pbtype in ['FIXED32', 'UINT32']:
				inner_init = str(self.default) + 'u'
			elif self.pbtype in ['FIXED64', 'UINT64']:
				inner_init = str(self.default) + 'ull'
			elif self.pbtype in ['SFIXED64', 'INT64']:
				inner_init = str(self.default) + 'll'
			else:
				inner_init = str(self.default)

		if inner_init_only:
			return inner_init

		outer_init = None
		if self.allocation == 'STATIC':
			if self.rules == 'REPEATED':
				if self.is_std_stl:
					outer_init = '{}'
				else:
					if self.use_stl is not None:
						outer_init = '{}'
					else:
						outer_init = ''
						if not self.fixed_count:
							outer_init += '0, '
						outer_init += '{'
						outer_init += ', '.join([inner_init] * self.max_count)
						outer_init += '}'
			elif self.rules == 'OPTIONAL':
				outer_init = 'false, ' + inner_init
			else:
				outer_init = inner_init
		elif self.allocation == 'POINTER':
			if self.rules == 'REPEATED':
				outer_init = '0, NULL'
			else:
				outer_init = 'NULL'
		elif self.allocation == 'CALLBACK':
			if self.pbtype == 'EXTENSION':
				outer_init = 'NULL'
			else:
				outer_init = '{{NULL}, NULL}'

		return outer_init

	def default_decl(self, declaration_only=False):
		'''Return definition for this field's default value.'''
		if self.default is None:
			return None

		ctype = self.ctype
		default = self.get_initializer(False, True)
		array_decl = ''

		if self.pbtype == 'STRING':
			if self.allocation != 'STATIC':
				return None  # Not implemented
			# change-begin max_size with enum
			if self.max_size_enum is not None:
				array_decl = '[%s]' % self.max_size_enum
			else:
				array_decl = '[%d]' % self.max_size
		# change-end
		elif self.pbtype == 'BYTES':
			if self.allocation != 'STATIC':
				return None  # Not implemented
		elif self.pbtype == 'FIXED_LENGTH_BYTES':
			if self.allocation != 'STATIC':
				return None  # Not implemented
			# change-begin max_size with enum
			if self.max_size_enum is not None:
				array_decl = '[%s]' % self.max_size_enum
			else:
				array_decl = '[%d]' % self.max_size
		# change-end

		if declaration_only:
			return 'extern const %s %s_default%s;' % (ctype, self.struct_name + self.name, array_decl)
		else:
			return 'const %s %s_default%s = %s;' % (ctype, self.struct_name + self.name, array_decl, default)

	def tags(self):
		'''Return the #define for the tag number of this field.'''
		identifier = '%s_%s_tag' % (self.struct_name, self.name)
		return '#define %-40s %d\n' % (identifier, self.tag)

	def pb_field_t(self, prev_field_name, union_index=None):
		'''Return the pb_field_t initializer to use in the constant array.
		prev_field_name is the name of the previous field or None. For OneOf
		unions, union_index is the index of this field inside the OneOf.
		'''

		if self.rules == 'ONEOF':
			if self.anonymous:
				result = '    PB_ANONYMOUS_ONEOF_FIELD(%s, ' % self.union_name
			else:
				result = '    PB_ONEOF_FIELD(%s, ' % self.union_name
		elif self.fixed_count:
			result = '    PB_REPEATED_FIXED_COUNT('
		else:
			result = '    PB_FIELD('

		result += '%3d, ' % self.tag
		result += '%-8s, ' % self.pbtype
		if not self.fixed_count:
			result += '%s, ' % self.rules
			result += '%-8s, ' % self.allocation

		if union_index is not None and union_index > 0:
			result += 'UNION, '
		elif prev_field_name is None:
			result += 'FIRST, '
		else:
			result += 'OTHER, '

		result += '%s, ' % self.struct_name
		result += '%s, ' % self.name
		result += '%s, ' % (prev_field_name or self.name)

		if self.pbtype == 'MESSAGE':
			result += '&%s_fields)' % self.submsgname
		elif self.default is None:
			result += '0)'
		elif self.pbtype in ['BYTES', 'STRING', 'FIXED_LENGTH_BYTES'] and self.allocation != 'STATIC':
			result += '0)'  # Arbitrary size default values not implemented
		elif self.rules == 'OPTEXT':
			result += '0)'  # Default value for extensions is not implemented
		else:
			result += '&%s_default)' % (self.struct_name + self.name)

		return result

	def get_last_field_name(self):
		return self.name

	def largest_field_value(self):
		'''Determine if this field needs 16bit or 32bit pb_field_t structure to compile properly.
		Returns numeric value or a C-expression for assert.'''
		check = []
		if self.pbtype == 'MESSAGE' and self.allocation == 'STATIC':
			if self.rules == 'REPEATED':
				check.append('pb_membersize(%s, %s[0])' % (self.struct_name, self.name))
			elif self.rules == 'ONEOF':
				if self.anonymous:
					check.append('pb_membersize(%s, %s)' % (self.struct_name, self.name))
				else:
					check.append('pb_membersize(%s, %s.%s)' % (self.struct_name, self.union_name, self.name))
			else:
				check.append('pb_membersize(%s, %s)' % (self.struct_name, self.name))
		elif self.pbtype == 'BYTES' and self.allocation == 'STATIC':
			if self.max_size > 251:
				check.append('pb_membersize(%s, %s)' % (self.struct_name, self.name))

		return FieldMaxSize([self.tag, self.max_size, self.max_count],
							check,
							('%s.%s' % (self.struct_name, self.name)))

	def encoded_size(self, dependencies):
		'''Return the maximum size that this field can take when encoded,
		including the field tag. If the size cannot be determined, returns
		None.'''

		if self.allocation != 'STATIC':
			return None

		if self.pbtype == 'MESSAGE':
			encsize = None
			if str(self.submsgname) in dependencies:
				submsg = dependencies[str(self.submsgname)]
				encsize = submsg.encoded_size(dependencies)
				if encsize is not None:
					# Include submessage length prefix
					encsize += varint_max_size(encsize.upperlimit())

			if encsize is None:
				# Submessage or its size cannot be found.
				# This can occur if submessage is defined in different
				# file, and it or its .options could not be found.
				# Instead of direct numeric value, reference the size that
				# has been #defined in the other file.
				encsize = EncodedSize(self.submsgname + 'size')

				# We will have to make a conservative assumption on the length
				# prefix size, though.
				encsize += 5

		elif self.pbtype in ['ENUM', 'UENUM']:
			if str(self.ctype) in dependencies:
				enumtype = dependencies[str(self.ctype)]
				encsize = enumtype.encoded_size()
			else:
				# Conservative assumption
				encsize = 10

		elif self.enc_size is None:
			raise RuntimeError("Could not determine encoded size for %s.%s"
							   % (self.struct_name, self.name))
		else:
			encsize = EncodedSize(self.enc_size)

		encsize += varint_max_size(self.tag << 3)  # Tag + wire type

		if self.rules == 'REPEATED':
			# Decoders must be always able to handle unpacked arrays.
			# Therefore we have to reserve space for it, even though
			# we emit packed arrays ourselves. For length of 1, packed
			# arrays are larger however so we need to add allowance
			# for the length byte.
			encsize *= self.max_count

			if self.max_count == 1:
				encsize += 1

		return encsize


class ExtensionRange(Field):
	def __init__(self, struct_name, range_start, field_options):
		'''Implements a special pb_extension_t* field in an extensible message
		structure. The range_start signifies the index at which the extensions
		start. Not necessarily all tags above this are extensions, it is merely
		a speed optimization.
		'''
		self.tag = range_start
		self.struct_name = struct_name
		self.name = 'extensions'
		self.pbtype = 'EXTENSION'
		self.rules = 'OPTIONAL'
		self.allocation = 'CALLBACK'
		self.ctype = 'pb_extension_t'
		self.array_decl = ''
		self.default = None
		self.max_size = 0
		self.max_count = 0
		self.fixed_count = False

	def __str__(self):
		return '    pb_extension_t *extensions;'

	def types(self):
		return ''

	def tags(self):
		return ''

	def encoded_size(self, dependencies):
		# We exclude extensions from the count, because they cannot be known
		# until runtime. Other option would be to return None here, but this
		# way the value remains useful if extensions are not used.
		return EncodedSize(0)


class ExtensionField(Field):
	def __init__(self, struct_name, desc, field_options):
		self.fullname = struct_name + desc.name
		self.extendee_name = names_from_type_name(desc.extendee)
		Field.__init__(self, self.fullname + 'struct', desc, field_options, False)

		if self.rules != 'OPTIONAL':
			self.skip = True
		else:
			self.skip = False
			self.rules = 'OPTEXT'

	def tags(self):
		'''Return the #define for the tag number of this field.'''
		identifier = '%s_tag' % self.fullname
		return '#define %-40s %d\n' % (identifier, self.tag)

	def extension_decl(self):
		'''Declaration of the extension type in the .pb.h file'''
		if self.skip:
			msg = '/* Extension field %s was skipped because only "optional"\n' % self.fullname
			msg += '   type of extension fields is currently supported. */\n'
			return msg

		return ('extern const pb_extension_type_t %s; /* field type: %s */\n' %
				(self.fullname, str(self).strip()))

	def extension_def(self):
		'''Definition of the extension type in the .pb.c file'''

		if self.skip:
			return ''

		result = 'typedef struct {\n'
		result += str(self)
		result += '\n} %s;\n\n' % self.struct_name
		result += ('static const pb_field_t %s_field = \n  %s;\n\n' %
				   (self.fullname, self.pb_field_t(None)))
		result += 'const pb_extension_type_t %s = {\n' % self.fullname
		result += '    NULL,\n'
		result += '    NULL,\n'
		result += '    &%s_field\n' % self.fullname
		result += '};\n'
		return result


# ---------------------------------------------------------------------------
#                   Generation of oneofs (unions)
# ---------------------------------------------------------------------------

class OneOf(Field):
	def __init__(self, struct_name, oneof_desc, options):
		self.options = options
		self.struct_name = struct_name
		self.name = oneof_desc.name
		self.ctype = 'union'
		self.pbtype = 'oneof'
		self.fields = []
		self.allocation = 'ONEOF'
		self.default = None
		self.rules = 'ONEOF'
		self.anonymous = False
		self.is_all_field_static = True

	def add_field(self, field):
		if field.allocation == 'CALLBACK':
			raise Exception("Callback fields inside of oneof are not supported"
							+ " (field %s)" % field.name)

		field.union_name = self.name
		field.rules = 'ONEOF'
		field.anonymous = self.anonymous
		self.fields.append(field)
		self.fields.sort(key=lambda f: f.tag)

		# Sort by the lowest tag number inside union
		self.tag = min([f.tag for f in self.fields])

		if not field.is_static():
			self.is_all_field_static = False

	def is_static(self):
		return self.is_all_field_static

	def __str__(self):
		result = ''
		if self.fields:
			result += '    pb_size_t which_' + self.name + ";\n"
			if self.anonymous:
				result += '    union {\n'
			else:
				result += '    union %s_%s {\n' % (self.struct_name, self.name)
				result += '        %s_%s() { }\n' % (self.struct_name, self.name)
				result += '        ~%s_%s() { }\n' % (self.struct_name, self.name)
				result += '        %s_%s(const %s_%s& _paramData) { }\n' % (self.struct_name, self.name, self.struct_name, self.name)
				result += '        %s_%s& operator=(const %s_%s& _paramData) { return *this; }\n' % (self.struct_name, self.name, self.struct_name, self.name)
			for f in self.fields:
				result += '    ' + str(f).replace('\n', '\n    ') + '\n'
			if self.anonymous:
				result += '    };'
			else:
				result += '    } ' + self.name + ';'
		return result

	def init_str(self):
		return '    which_%s = 0;\n' % self.name

	def init_which_str(self):
		result = ''
		result += '    if (which_%s != 0) {\n' % self.name
		result += '        if (NULL != g_nanopb_frompb_log_handle) {\n'
		result += '            g_nanopb_frompb_log_handle("the which_%s has inited:%%d, now you select:%%d", which_%s, which_data);\n' % (self.name, self.name)
		result += '        }\n'
		result += '        uninit_which_%s();\n' % self.name
		result += '    }\n\n'
		result += '\n'
		if self.fields:
			result += '    switch (which_data)\n'
			result += '    {\n'
			for f in self.fields:
				result += '    case %s_%s_tag:\n' % (f.struct_name, f.name)
				result += self.oneof_field_init_which_str(f)
				result += '        break;\n'
			result += '    case 0:\n'
			result += '        break;\n'
			result += '    }\n'
			result += '    which_%s = which_data;\n' % self.name
		return result

	def oneof_field_init_which_str(self, f):
		result = ''
		if self.anonymous:
			if f.pbtype == 'STRING' or f.pbtype == 'BYTES' or f.pbtype == 'FIXED_LENGTH_BYTES':
				if f.use_stl_string:
					result += '        new(&%s) %s();\n' % (f.name, f.ctype)
				else:
					if f.is_std_stl:
						result += '        new(&%s) std::string();\n' % f.name
					else:
						if f.pbtype == 'STRING':
							result += '        %s[0] = \'\\0\';\n' % f.name
						elif f.pbtype == 'BYTES':
							result += '        %s.size = 0;\n' % f.name
						elif f.pbtype == 'FIXED_LENGTH_BYTES':
							result += '        memset(%s, 0, sizeof(%s));\n' % (f.name, f.name)
			elif f.pbtype == 'MESSAGE':
				result += '        new(&%s) %s();\n' % (f.name, f.ctype)
			elif f.pbtype in ('ENUM', 'UENUM'):
				result = '        %s = _%s_MIN;\n' % (f.name, f.ctype)
			else:
				result += '        %s = 0;\n' % f.name
		else:
			if f.pbtype == 'STRING' or f.pbtype == 'BYTES' or f.pbtype == 'FIXED_LENGTH_BYTES':
				if f.use_stl_string:
					result += '        new(&%s.%s) %s();\n' % (self.name, f.name, f.ctype)
				else:
					if f.is_std_stl:
						result += '        new(&%s.%s) std::string();\n' % (self.name, f.name)
					else:
						if f.pbtype == 'STRING':
							result += '        %s.%s[0] = \'\\0\';\n' % (self.name, f.name)
						elif f.pbtype == 'BYTES':
							result += '        %s.%s.size = 0;\n' % (self.name, f.name)
						elif f.pbtype == 'FIXED_LENGTH_BYTES':
							result += '        memset(%s.%s, 0, sizeof(%s.%s));\n' % (self.name, f.name, self.name, f.name)
			elif f.pbtype == 'MESSAGE':
				result += '        new(&%s.%s) %s();\n' % (self.name, f.name, f.ctype)
			elif f.pbtype in ('ENUM', 'UENUM'):
				result = '        %s.%s = _%s_MIN;\n' % (self.name, f.name, f.ctype)
			else:
				result += '        %s.%s = 0;\n' % (self.name, f.name)
		return result

	def oneof_field_from_pb_str(self, f):
		result = ''
		if self.anonymous:
			if f.is_std_stl:
				if f.pbtype == 'STRING' or f.pbtype == 'BYTES' or f.pbtype == 'FIXED_LENGTH_BYTES':
					result += '        %s = cc.%s();\n' % (f.name, f.name.lower())
				elif f.pbtype == 'MESSAGE':
					result += '        if (!%s.FromPb(cc.%s())) return false;\n' % (f.name, f.name.lower())
				elif f.pbtype in ('ENUM', 'UENUM'):
					result += '        %s = static_cast<%s>(cc.%s());\n' % (f.name, f.ctype, f.name.lower())
				else:
					result += '        %s = cc.%s();\n' % (f.name, f.name.lower())
			else:
				if f.pbtype == 'STRING':
					result += '        if (cc.%s().length() >= %u)\n' % (f.name.lower(), f.max_size)
					result += '        {\n'
					result += '            if (NULL != g_nanopb_frompb_log_handle)\n'
					result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur length:%%d, max length:%s", cc.%s().length());\n' % (self.struct_name, f.name, f.max_size - 1, f.name.lower())
					result += '            return false;\n'
					result += '        }\n'
					if f.use_stl_string:
						result += '        %s = cc.%s();\n' % (f.name, f.name.lower())
					else:
						if f.max_size_enum is not None:
							result += '        snprintf(%s, %s, "%%s", cc.%s().c_str());\n' % (f.name, f.max_size_enum, f.name.lower())
						else:
							result += '        snprintf(%s, %u, "%%s", cc.%s().c_str());\n' % (f.name, f.max_size, f.name.lower())
				elif f.pbtype == 'BYTES':
					result += '        if (cc.%s().size() > %u)\n' % (f.name.lower(), f.max_size)
					result += '        {\n'
					result += '            if (NULL != g_nanopb_frompb_log_handle)\n'
					result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur size:%%d, max size:%s", cc.%s().size());\n' % (self.struct_name, f.name, f.max_size, f.name.lower())
					result += '            return false;\n'
					result += '        }\n'
					if f.use_stl_string:
						result += '        %s = cc.%s();\n' % (f.name, f.name.lower())
					else:
						result += '        memcpy(%s.bytes, cc.%s().data(), cc.%s().size());\n' % (f.name, f.name.lower(), f.name.lower())
						result += '        %s.size = cc.%s().size();\n' % (f.name, f.name.lower())
				elif f.pbtype == 'FIXED_LENGTH_BYTES':
					result += '        if (cc.%s().size() > %u)\n' % (f.name.lower(), f.max_size)
					result += '        {\n'
					result += '            if (NULL != g_nanopb_frompb_log_handle)\n'
					result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur size:%%d, max size:%s", cc.%s().size());\n' % (self.struct_name, f.name, f.max_size, f.name.lower())
					result += '            return false;\n'
					result += '        }\n'
					if f.use_stl_string:
						result += '        %s = cc.%s();\n' % (f.name, f.name.lower())
					else:
						result += '        memcpy(%s, cc.%s().data(), cc.%s().size());\n' % (f.name, f.name.lower(), f.name.lower())
				elif f.pbtype == 'MESSAGE':
					result += '        if (!%s.FromPb(cc.%s())) return false;\n' % (f.name, f.name.lower())
				elif f.pbtype in ('ENUM', 'UENUM'):
					result += '        %s = static_cast<%s>(cc.%s());\n' % (f.name, f.ctype, f.name.lower())
				else:
					result += '        %s = cc.%s();\n' % (f.name, f.name.lower())
		else:
			if f.is_std_stl:
				if f.pbtype == 'STRING' or f.pbtype == 'BYTES' or f.pbtype == 'FIXED_LENGTH_BYTES':
					result += '        %s.%s = cc.%s();\n' % (self.name, f.name, f.name.lower())
				elif f.pbtype == 'MESSAGE':
					result += '        if (!%s.%s.FromPb(cc.%s())) return false;\n' % (self.name, f.name, f.name.lower())
				elif f.pbtype in ('ENUM', 'UENUM'):
					result += '        %s.%s = static_cast<%s>(cc.%s());\n' % (self.name, f.name, f.ctype, f.name.lower())
				else:
					result += '        %s.%s = cc.%s();\n' % (self.name, f.name, f.name.lower())
			else:
				if f.pbtype == 'STRING':
					result += '        if (cc.%s().length() >= %u)\n' % (f.name.lower(), f.max_size)
					result += '        {\n'
					result += '            if (NULL != g_nanopb_frompb_log_handle)\n'
					result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur length:%%d, max length:%s", cc.%s().length());\n' % (self.struct_name, f.name, f.max_size - 1, f.name.lower())
					result += '            return false;\n'
					result += '        }\n'
					if f.use_stl_string:
						result += '        %s.%s = cc.%s();\n' % (self.name, f.name, f.name.lower())
					else:
						if f.max_size_enum is not None:
							result += '        snprintf(%s.%s, %s, "%%s", cc.%s().c_str());\n' % (self.name, f.name, f.max_size_enum, f.name.lower())
						else:
							result += '        snprintf(%s.%s, %u, "%%s", cc.%s().c_str());\n' % (self.name, f.name, f.max_size, f.name.lower())
				elif f.pbtype == 'BYTES':
					result += '        if (cc.%s().size() > %u)\n' % (f.name.lower(), f.max_size)
					result += '        {\n'
					result += '            if (NULL != g_nanopb_frompb_log_handle)\n'
					result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur size:%%d, max size:%s", cc.%s().size());\n' % (self.struct_name, f.name, f.max_size, f.name.lower())
					result += '            return false;\n'
					result += '        }\n'
					if f.use_stl_string:
						result += '        %s.%s = cc.%s();\n' % (self.name, f.name, f.name.lower())
					else:
						result += '        memcpy(%s.%s.bytes, cc.%s().data(), cc.%s().size());\n' % (self.name, f.name, f.name.lower(), f.name.lower())
						result += '        %s.%s.size = cc.%s().size();\n' % (self.name, f.name, f.name.lower())
				elif f.pbtype == 'FIXED_LENGTH_BYTES':
					result += '        if (cc.%s().size() > %u)\n' % (f.name.lower(), f.max_size)
					result += '        {\n'
					result += '            if (NULL != g_nanopb_frompb_log_handle)\n'
					result += '                g_nanopb_frompb_log_handle("struct:%s, field:%s, cur size:%%d, max size:%s", cc.%s().size());\n' % (self.struct_name, f.name, f.max_size, f.name.lower())
					result += '            return false;\n'
					result += '        }\n'
					if f.use_stl_string:
						result += '        %s.%s = cc.%s();\n' % (self.name, f.name, f.name.lower())
					else:
						result += '        memcpy(%s.%s, cc.%s().data(), cc.%s().size());\n' % (self.name, f.name, f.name.lower(), f.name.lower())
				elif f.pbtype == 'MESSAGE':
					result += '        if (!%s.%s.FromPb(cc.%s())) return false;\n' % (self.name, f.name, f.name.lower())
				elif f.pbtype in ('ENUM', 'UENUM'):
					result += '        %s.%s = static_cast<%s>(cc.%s());\n' % (self.name, f.name, f.ctype, f.name.lower())
				else:
					result += '        %s.%s = cc.%s();\n' % (self.name, f.name, f.name.lower())
		return result

	def from_pb_str(self):
		result = ''
		if self.fields:
			result += '    switch (cc.%s_case())\n' % self.name
			result += '    {\n'
			for f in self.fields:
				result += '    case %s_%s_tag:\n' % (f.struct_name, f.name)
				result += self.oneof_field_from_pb_str(f)
				result += '        break;\n'
			result += '    case 0:\n'
			result += '        break;\n'
			result += '    }\n'
			result += '    which_%s = cc.%s_case();\n' % (self.name, self.name)
		return result

	def destruct_str(self):
		return '    uninit_which_%s();\n' % self.name

	def uninit_which_str(self):
		result = ''
		if self.fields:
			result += '    switch (which_%s)\n' % self.name
			result += '    {\n'
			for f in self.fields:
				result += '    case %s_%s_tag:\n' % (f.struct_name, f.name)
				result += self.oneof_field_uninit_which_str(f)
				result += '        break;\n'
			result += '    case 0:\n'
			result += '        break;\n'
			result += '    }\n'
		return result

	def oneof_field_uninit_which_str(self, f):
		result = ''
		if self.anonymous:
			if f.pbtype == 'STRING' or f.pbtype == 'BYTES' or f.pbtype == 'FIXED_LENGTH_BYTES':
				if f.use_stl_string:
					result += '        %s.~%s();\n' % (f.name, f.ctype)
				else:
					if f.is_std_stl:
						result += '        %s.~std::string();\n' % f.name
			elif f.pbtype == 'MESSAGE':
				result += '        %s.~%s();\n' % (f.name, f.ctype)
		else:
			if f.pbtype == 'STRING' or f.pbtype == 'BYTES' or f.pbtype == 'FIXED_LENGTH_BYTES':
				if f.use_stl_string:
					result += '        %s.%s.~%s();\n' % (self.name, f.name, f.ctype)
				else:
					if f.is_std_stl:
						result += '        %s.%s.~std::string();\n' % (self.name, f.name)
			elif f.pbtype == 'MESSAGE':
				result += '        %s.%s.~%s();\n' % (self.name, f.name, f.ctype)
		return result

	def resumeinit_str(self):
		result = ''
		if self.fields:
			result += '    switch (which_%s)\n' % self.name
			result += '    {\n'
			for f in self.fields:
				result += '    case %s_%s_tag:\n' % (f.struct_name, f.name)
				result += self.oneof_field_resumeinit_str(f)
				result += '        break;\n'
			result += '    case 0:\n'
			result += '        break;\n'
			result += '    }\n'
		return result

	def oneof_field_resumeinit_str(self, f):
		result = ''
		if self.anonymous:
			if f.pbtype == 'STRING' or f.pbtype == 'BYTES' or f.pbtype == 'FIXED_LENGTH_BYTES':
				if f.use_stl_string:
					result += '        new(&%s) %s();\n' % (f.name, f.ctype)
				else:
					if f.is_std_stl:
						result += '        new(&%s) std::string();\n' % f.name
			elif f.pbtype == 'MESSAGE':
				result += '        new(&%s) %s();\n' % (f.name, f.ctype)
		else:
			if f.pbtype == 'STRING' or f.pbtype == 'BYTES' or f.pbtype == 'FIXED_LENGTH_BYTES':
				if f.use_stl_string:
					result += '        new(&%s.%s) %s();\n' % (self.name, f.name, f.ctype)
				else:
					if f.is_std_stl:
						result += '        new(&%s.%s) std::string();\n' % (self.name, f.name)
			elif f.pbtype == 'MESSAGE':
				result += '        new(&%s.%s) %s();\n' % (self.name, f.name, f.ctype)
		return result

	def copy_data_str(self):
		result = ''
		result += '    init_which_%s(stArgsData.which_%s);\n' % (self.name, self.name)
		if self.fields:
			result += '    switch (stArgsData.which_%s)\n' % self.name
			result += '    {\n'
			for f in self.fields:
				result += '    case %s_%s_tag:\n' % (f.struct_name, f.name)
				result += self.oneof_field_copy_data_str(f)
				result += '        break;\n'
			result += '    case 0:\n'
			result += '        break;\n'
			result += '    }\n'
		return result

	def oneof_field_copy_data_str(self, f):
		result = ''
		if self.anonymous:
			if f.is_std_stl:
				result += '        %s = stArgsData.%s;\n' % (f.name, f.name)
			else:
				if f.pbtype == 'STRING':
					if f.use_stl_string:
						result += '        %s = stArgsData.%s;\n' % (f.name, f.name)
					else:
						if f.max_size_enum is not None:
							result += '        snprintf(%s, %s, "%%s", stArgsData.%s);\n' % (f.name, f.max_size_enum, f.name)
						else:
							result += '        snprintf(%s, %u, "%%s", stArgsData.%s);\n' % (f.name, f.max_size, f.name)
				elif f.pbtype == 'BYTES':
					if f.use_stl_string:
						result += '        %s = stArgsData.%s;\n' % (f.name, f.name)
					else:
						result += '        memcpy(%s.bytes, stArgsData.%s.bytes, stArgsData.%s.size);\n' % (f.name, f.name, f.name)
						result += '        %s.size = stArgsData.%s.size;\n' % (f.name, f.name)
				elif f.pbtype == 'FIXED_LENGTH_BYTES':
					if f.use_stl_string:
						result += '        %s = stArgsData.%s;\n' % (f.name, f.name)
					else:
						result += '        memcpy(%s, stArgsData.%s, sizeof(%s));\n' % (f.name, f.name, f.name)
				else:
					result += '        %s = stArgsData.%s;\n' % (f.name, f.name)
		else:
			if f.is_std_stl:
				result += '        %s.%s = stArgsData.%s.%s;\n' % (self.name, f.name, self.name, f.name)
			else:
				if f.pbtype == 'STRING':
					if f.use_stl_string:
						result += '        %s.%s = stArgsData.%s.%s;\n' % (self.name, f.name, self.name, f.name)
					else:
						if f.max_size_enum is not None:
							result += '        snprintf(%s.%s, %s, "%%s", stArgsData.%s.%s);\n' % (self.name, f.name, f.max_size_enum, self.name, f.name)
						else:
							result += '        snprintf(%s.%s, %u, "%%s", stArgsData.%s.%s);\n' % (self.name, f.name, f.max_size, self.name, f.name)
				elif f.pbtype == 'BYTES':
					if f.use_stl_string:
						result += '        %s.%s = stArgsData.%s.%s;\n' % (self.name, f.name, self.name, f.name)
					else:
						result += '        memcpy(%s.%s.bytes, stArgsData.%s.%s.bytes, stArgsData.%s.%s.size);\n' % (self.name, f.name, self.name, f.name, self.name, f.name)
						result += '        %s.%s.size = stArgsData.%s.%s.size;\n' % (self.name, f.name, self.name, f.name)
				elif f.pbtype == 'FIXED_LENGTH_BYTES':
					if f.use_stl_string:
						result += '        %s.%s = stArgsData.%s.%s;\n' % (self.name, f.name, self.name, f.name)
					else:
						result += '        memcpy(%s.%s, stArgsData.%s.%s, sizeof(%s.%s));\n' % (self.name, f.name, self.name, f.name, self.name, f.name)
				else:
					result += '        %s.%s = stArgsData.%s.%s;\n' % (self.name, f.name, self.name, f.name)
		return result

	def oneof_field_to_pb_str(self, f):
		result = ''
		if self.anonymous:
			if f.pbtype == 'STRING':
				if f.use_stl_string:
					result += '        cc->set_%s(%s.data());\n' % (f.name.lower(), f.name)
				else:
					result += '        cc->set_%s(%s);\n' % (f.name.lower(), f.name)
			elif f.pbtype == 'BYTES':
				if f.use_stl_string:
					result += '        cc->set_%s(%s.data(), %s.length());\n' % (f.name.lower(), f.name, f.name)
				else:
					result += '        cc->set_%s(%s.bytes, %s.size);\n' % (f.name.lower(), f.name, f.name)
			elif f.pbtype == 'FIXED_LENGTH_BYTES':
				if f.use_stl_string:
					result += '        cc->set_%s(%s.data(), %s.length());\n' % (f.name.lower(), f.name, f.name)
				else:
					result += '        cc->set_%s(%s, %u);\n' % (f.name.lower(), f.name, f.max_size)
			elif f.pbtype == 'MESSAGE':
				result += '            %s.ToPb(cc->mutable_%s());\n' % (f.name, f.name.lower())
			elif f.pbtype in ('ENUM', 'UENUM'):
				result = '        cc->set_%s(static_cast<%s>(%s));\n' % (f.name.lower(), namespace_from_type_name(f.type_name), f.name)
			else:
				result += '        cc->set_%s(%s);\n' % (f.name.lower(), f.name)
		else:
			if f.pbtype == 'STRING':
				if f.use_stl_string:
					result += '        cc->set_%s(%s.%s.data());\n' % (f.name.lower(), self.name, f.name)
				else:
					result += '        cc->set_%s(%s.%s);\n' % (f.name.lower(), self.name, f.name)
			elif f.pbtype == 'BYTES':
				if f.use_stl_string:
					result += '        cc->set_%s(%s.%s.data(), %s.%s.length());\n' % (f.name.lower(), self.name, f.name, self.name, f.name)
				else:
					result += '        cc->set_%s(%s.%s.bytes, %s.%s.size);\n' % (f.name.lower(), self.name, f.name, self.name, f.name)
			elif f.pbtype == 'FIXED_LENGTH_BYTES':
				if f.use_stl_string:
					result += '        cc->set_%s(%s.%s.data(), %s.%s.length());\n' % (f.name.lower(), self.name, f.name, self.name, f.name)
				else:
					result += '        cc->set_%s(%s.%s, %u);\n' % (f.name.lower(), self.name, f.name, f.max_size)
			elif f.pbtype == 'MESSAGE':
				result += '        %s.%s.ToPb(cc->mutable_%s());\n' % (self.name, f.name, f.name.lower())
			elif f.pbtype in ('ENUM', 'UENUM'):
				result = '        cc->set_%s(static_cast<%s>(%s.%s));\n' % (f.name.lower(), namespace_from_type_name(f.type_name), self.name, f.name)
			else:
				result += '        cc->set_%s(%s.%s);\n' % (f.name.lower(), self.name, f.name)
		return result

	def to_pb_str(self):
		result = ''
		if self.fields:
			result += '    switch (which_%s)\n' % self.name
			result += '    {\n'
			for f in self.fields:
				result += '    case %s_%s_tag:\n' % (f.struct_name, f.name)
				result += self.oneof_field_to_pb_str(f)
				result += '        break;\n'
			result += '    }\n'
		return result

	def oneof_field_short_debug_string_str(self, f):
		result = ''
		if self.anonymous:
			result += '        ss << "%s" << ":"' % f.name
			if f.pbtype == 'BYTES':
				result += ' << "(skip print BYTES)";\n'
			elif f.pbtype == 'FIXED_LENGTH_BYTES':
				result += ' << "(skip print FIXED_LENGTH_BYTES)";\n'
			elif f.pbtype == 'MESSAGE':
				result += ' << %s.ShortDebugString();\n' % f.name
			elif f.pbtype == 'STRING':
				if f.is_std_stl:
					result += ' << "\\"" << %s << "\\"";\n' % f.name
				else:
					if f.use_stl_string:
						result += ' << "\\"" << %s.c_str() << "\\"";\n' % f.name
					else:
						result += ' << "\\"" << %s << "\\"";\n' % f.name
			else:
				if self.ctype == "int8_t":
					result += ' << static_cast<int>(%s);\n' % f.name
				elif self.ctype == "uint8_t":
					result += ' << static_cast<unsigned int>(%s);\n' % f.name
				else:
					result += ' << %s;\n' % f.name
		else:
			result += '        ss << "%s.%s" << ":"' % (self.name, f.name)
			if f.pbtype == 'BYTES':
				result += ' << "(skip print BYTES)";\n'
			elif f.pbtype == 'FIXED_LENGTH_BYTES':
				result += ' << "(skip print FIXED_LENGTH_BYTES)";\n'
			elif f.pbtype == 'MESSAGE':
				result += ' << %s.%s.ShortDebugString();\n' % (self.name, f.name)
			elif f.pbtype == 'STRING':
				if f.is_std_stl:
					result += ' << "\\"" << %s.%s << "\\"";\n' % (self.name, f.name)
				else:
					if f.use_stl_string:
						result += ' << "\\"" << %s.%s.c_str() << "\\"";\n' % (self.name, f.name)
					else:
						result += ' << "\\"" << %s.%s << "\\"";\n' % (self.name, f.name)
			else:
				if self.ctype == "int8_t":
					result += ' << static_cast<int>(%s.%s);\n' % (self.name, f.name)
				elif self.ctype == "uint8_t":
					result += ' << static_cast<unsigned int>(%s.%s);\n' % (self.name, f.name)
				else:
					result += ' << %s.%s;\n' % (self.name, f.name)
		return result

	def short_debug_string_str(self):
		result = ''
		if self.fields:
			result += '    switch (which_%s)\n' % self.name
			result += '    {\n'
			for f in self.fields:
				result += '    case %s_%s_tag:\n' % (f.struct_name, f.name)
				result += self.oneof_field_short_debug_string_str(f)
				result += '        break;\n'
			result += '    }\n'
		return result

	def get_which_declaration_str(self):
		result = 'public:\n'
		result += '    void init_which_%s(pb_size_t which_data);\n' % self.name
		result += '    void uninit_which_%s();\n' % self.name
		result += '    pb_size_t get_which_%s() const { return which_%s; }\n' % (self.name, self.name)
		for f in self.fields:
			result += '    %s* get_%s_%s();\n' % (f.ctype, self.name, f.name)
			result += '    const %s* get_%s_%s() const;\n' % (f.ctype, self.name, f.name)
		return result

	def get_which_definition_str(self):
		result = ''
		result += 'void %s::init_which_%s(pb_size_t which_data) {\n' % (self.struct_name, self.name)
		result += self.init_which_str()
		result += '}\n\n'
		result += 'void %s::uninit_which_%s() {\n' % (self.struct_name, self.name)
		result += self.uninit_which_str()
		result += '}\n\n'
		for f in self.fields:
			result += '%s* %s::get_%s_%s() {\n' % (f.ctype, f.struct_name, self.name, f.name)
			result += '    if (which_%s == %s_%s_tag) {\n' % (self.name, f.struct_name, f.name)
			result += self.oneof_field_get_which_definition_str(f)
			result += '    }\n'
			result += '    if (NULL != g_nanopb_frompb_log_handle) {\n'
			result += '        g_nanopb_frompb_log_handle("the which_%s:%%d != %s_%s_tag, so return NULL", which_%s);\n' % (self.name, f.struct_name, f.name, self.name)
			result += '    }\n'
			result += '    return NULL;\n'
			result += '}\n\n'
			result += 'const %s* %s::get_%s_%s() const {\n' % (f.ctype, f.struct_name, self.name, f.name)
			result += '    if (which_%s == %s_%s_tag) {\n' % (self.name, f.struct_name, f.name)
			result += self.oneof_field_get_which_definition_str(f)
			result += '    }\n'
			result += '    if (NULL != g_nanopb_frompb_log_handle) {\n'
			result += '        g_nanopb_frompb_log_handle("the which_%s:%%d != %s_%s_tag, so return NULL", which_%s);\n' % (self.name, f.struct_name, f.name, self.name)
			result += '    }\n'
			result += '    return NULL;\n'
			result += '}\n\n'
		return result

	def oneof_field_get_which_definition_str(self, f):
		result = ''
		if self.anonymous:
			if f.is_std_stl:
				result += '        return &%s;\n' % f.name
			else:
				if f.pbtype == 'STRING':
					if f.use_stl_string:
						result += '        return &%s;\n' % f.name
					else:
						result += '        return %s;\n' % f.name
				elif f.pbtype == 'BYTES':
					result += '        return &%s;\n' % f.name
				elif f.pbtype == 'FIXED_LENGTH_BYTES':
					if f.use_stl_string:
						result += '        return &%s;\n' % f.name
					else:
						result += '        return %s;\n' % f.name
				else:
					result += '        return &%s;\n' % f.name
		else:
			if f.is_std_stl:
				result += '        return &%s.%s;\n' % (self.name, f.name)
			else:
				if f.pbtype == 'STRING':
					if f.use_stl_string:
						result += '        return &%s.%s;\n' % (self.name, f.name)
					else:
						result += '        return %s.%s;\n' % (self.name, f.name)
				elif f.pbtype == 'BYTES':
					result += '        return &%s.%s;\n' % (self.name, f.name)
				elif f.pbtype == 'FIXED_LENGTH_BYTES':
					if f.use_stl_string:
						result += '        return &%s.%s;\n' % (self.name, f.name)
					else:
						result += '        return %s.%s;\n' % (self.name, f.name)
				else:
					result += '        return &%s.%s;\n' % (self.name, f.name)
		return result

	def createinit_str(self):
		return '    which_%s = 0;\n' % self.name

	def types(self):
		return ''.join([f.types() for f in self.fields])

	def get_dependencies(self):
		deps = []
		for f in self.fields:
			deps += f.get_dependencies()
		return deps

	def get_initializer(self, null_init):
		return '0, {' + self.fields[0].get_initializer(null_init) + '}'

	def default_decl(self, declaration_only=False):
		return None

	def tags(self):
		return ''.join([f.tags() for f in self.fields])

	def pb_field_t(self, prev_field_name):
		parts = []
		for union_index, field in enumerate(self.fields):
			parts.append(field.pb_field_t(prev_field_name, union_index))
		return ',\n'.join(parts)

	def get_last_field_name(self):
		if self.anonymous:
			return self.fields[-1].name
		else:
			return self.name + '.' + self.fields[-1].name

	def largest_field_value(self):
		largest = FieldMaxSize()
		for f in self.fields:
			largest.extend(f.largest_field_value())
		return largest

	def encoded_size(self, dependencies):
		return 'hjhkklkll'

	def encoded_size_bak(self, dependencies):
		'''Returns the size of the largest oneof field.'''
		largest = EncodedSize(0)
		symbols = set()
		for f in self.fields:
			size = EncodedSize(f.encoded_size(dependencies))
			if size.value is None:
				return None
			elif size.symbols:
				symbols.add(EncodedSize(f.submsgname + 'size').symbols[0])
			elif size.value > largest.value:
				largest = size

		if not symbols:
			return largest

		symbols = list(symbols)
		symbols.append(str(largest))
		max_size = lambda x, y: '({0} > {1} ? {0} : {1})'.format(x, y)
		return reduce(max_size, symbols)


# ---------------------------------------------------------------------------
#                   Generation of messages (structures)
# ---------------------------------------------------------------------------


class Message:
	def __init__(self, names, desc, message_options, cctype, options):
		self.cctype = cctype
		self.name = names
		self.options = options
		self.fields = []
		self.fields_map = {}
		self.oneofs = {}
		no_unions = []
		# change-begin
		self.insert_msg_last = message_options.insert_msg_last
		self.stl_keys = []
		# change-end
		self.all_fields_static = True
		self.is_std_stl = False
		self.to_db_sql = False

		if message_options.msgid:
			self.msgid = message_options.msgid

		if message_options.to_db_sql:
			self.to_db_sql = message_options.to_db_sql
			nfshmHeaderFileList.append("#include \"NFComm/NFObjCommon/NFSeqOP.h\"")

		if message_options.is_std_stl:
			self.is_std_stl = message_options.is_std_stl

		if hasattr(desc, 'oneof_decl'):
			for i, f in enumerate(desc.oneof_decl):
				oneof_options = get_nanopb_suboptions(desc, message_options, self.name + f.name)
				if oneof_options.no_unions:
					no_unions.append(i)  # No union, but add fields normally
				elif oneof_options.type == nanopb_pb2.FT_IGNORE:
					pass  # No union and skip fields also
				else:
					oneof = OneOf(self.name, f, options)
					if oneof_options.anonymous_oneof:
						oneof.anonymous = True
					self.oneofs[i] = oneof
					self.fields.append(oneof)
		else:
			sys.stderr.write('Note: This Python protobuf library has no OneOf support\n')

		for f in desc.field:
			field_options = get_nanopb_suboptions(f, message_options, self.name + f.name)
			if field_options.type == nanopb_pb2.FT_IGNORE:
				continue

			field = Field(self.name, f, field_options, self.is_std_stl)
			if (hasattr(f, 'oneof_index') and
					f.HasField('oneof_index') and
					f.oneof_index not in no_unions):
				if f.oneof_index in self.oneofs:
					self.oneofs[f.oneof_index].add_field(field)
			else:
				self.fields.append(field)
				temp_field_name = '%s' % f.name
				self.fields_map[temp_field_name] = field

			# change-begin
			if field.is_stl_key:
				self.stl_keys.append(field.name)
			# change-end

		if len(desc.extension_range) > 0:
			field_options = get_nanopb_suboptions(desc, message_options, self.name + 'extensions')
			range_start = min([r.start for r in desc.extension_range])
			if field_options.type != nanopb_pb2.FT_IGNORE:
				self.fields.append(ExtensionRange(self.name, range_start, field_options))

		self.packed = message_options.packed_struct
		self.ordered_fields = self.fields[:]
		self.ordered_fields.sort()

		for f in self.fields:
			if not f.is_static():
				self.all_fields_static = False
				break

	def get_dependencies(self):
		'''Get list of type names that this structure refers to.'''
		deps = []
		for f in self.fields:
			deps += f.get_dependencies()
		return deps

	def __str__(self):
		if self.options.source_extension == '.cc':
			if self.to_db_sql:
				result = 'struct %s : public NFDescStoreSeqOP {\n' % self.name
			else:
				result = 'struct %s {\n' % self.name
		else:
			if self.to_db_sql:
				result = 'typedef struct _%s : public NFDescStoreSeqOP {\n' % self.name
			else:
				result = 'typedef struct _%s {\n' % self.name

		if not self.ordered_fields:
			# Empty structs are not allowed in C standard.
			# Therefore add a dummy field if an empty message occurs.
			result += '    char dummy_field;'

		result += '\n'.join([str(f) for f in self.ordered_fields])
		# change-begin
		if len(self.insert_msg_last) > 0:
			result += '\n    ' + self.insert_msg_last
		# change-end

		if self.options.source_extension == '.cc' and self.all_fields_static:
			# if self.options.source_extension == '.cc':
			result += '\n\n' + self.construct_declaration_str()
			result += self.destruct_declaration_str()
			result += self.copy_construct_declaration_str()
			result += self.copy_operator_declaration_str()
			result += self.copy_data_declaration_str()
			result += self.createinit_declaration_str()
			result += self.resumeinit_declaration_str()
			result += self.init_declaration_str()
			result += self.from_pb_declaration_str()
			result += self.to_pb_declaration_str()
			result += self.short_debug_string_declaration_str()
			result += self.get_which_declaration_str()

			if len(self.stl_keys) > 0:
				result += "\n\tbool operator==(const %s &value) const\n" % self.name
				result += "\t{\n"
				result += "\t\treturn "
				for i in range(len(self.stl_keys)):
					result += "%s == value.%s" % (self.stl_keys[i], self.stl_keys[i])
					if i != len(self.stl_keys) - 1:
						result += " && "
				result += ";\n"
				result += "\t}\n"

			if len(self.stl_keys) > 0:
				result += "\n\tbool operator<(const %s &value) const\n" % self.name
				result += "\t{\n"
				result += "\t\treturn "
				for i in range(len(self.stl_keys)):
					result += "%s < value.%s" % (self.stl_keys[i], self.stl_keys[i])
					if i != len(self.stl_keys) - 1:
						result += " && "
				result += ";\n"
				result += "\t}\n"

		result += '\n/* @@protoc_insertion_point(struct:%s) */' % self.name
		result += '\n}'

		if self.packed:
			result += ' pb_packed'

		if self.options.source_extension == '.cc':
			result += ';'
		else:
			result += ' %s;' % self.name

		if self.packed:
			result = 'PB_PACKED_STRUCT_START\n' + result
			result += '\nPB_PACKED_STRUCT_END'

		# change-begin
		if len(self.stl_keys) > 0:
			result += "\n\n"
			result += "namespace std\n"
			result += "{\n"
			result += "\ttemplate<>\n"
			result += "\tstruct hash<%s>\n" % self.name
			result += "\t{\n"
			result += "\t\tsize_t operator()(const %s &value) const\n" % self.name
			result += "\t\t{\n"
			result += "\t\t\treturn NFHash::hash_combine("
			for i in range(len(self.stl_keys)):
				result += "value.%s" % self.stl_keys[i];
				if i != len(self.stl_keys) - 1:
					result += ","

			result += ");\n"
			result += "\t\t}\n"
			result += "\t};\n"
			result += "}\n"
		# change-end

		return result

	def construct_declaration_str(self):
		return '\t%s();\n' % self.name

	def construct_definition_str(self):
		result = '%s::%s()\n' % (self.name, self.name)
		result += '{\n'
		result += '\tif (EN_OBJ_MODE_INIT == NFShmMgr::Instance()->GetCreateMode()) {\n'
		result += '\t\tCreateInit();\n'
		result += '\t} else {\n'
		result += '\t\tResumeInit();\n'
		result += '\t}\n'
		result += '}\n\n'
		return result

	def copy_construct_declaration_str(self):
		return '\t%s(const %s& stArgsData);\n' % (self.name, self.name)

	def copy_construct_definition_str(self):
		result = '%s::%s(const %s& stArgsData)\n' % (self.name, self.name, self.name)
		result += '{\n'
		if len(self.oneofs) > 0:
			result += '\tif (EN_OBJ_MODE_INIT == NFShmMgr::Instance()->GetCreateMode()) {\n'
			result += '\t\tCreateInit();\n'
			result += '\t} else {\n'
			result += '\t\tResumeInit();\n'
			result += '\t}\n'
		result += '\tif (this != &stArgsData) {\n'
		result += '\t\tCopyData(stArgsData);\n'
		result += '\t}\n'
		result += '}\n\n'
		return result

	def copy_operator_declaration_str(self):
		return '\t%s& operator=(const %s& stArgsData);\n' % (self.name, self.name)

	def copy_operator_definition_str(self):
		result = '%s& %s::operator=(const %s& stArgsData)\n' % (self.name, self.name, self.name)
		result += '{\n'
		result += '\tif (this != &stArgsData) {\n'
		result += '\t\tCopyData(stArgsData);\n'
		result += '\t}\n'
		result += '\treturn *this;\n'
		result += '}\n\n'
		return result

	def copy_data_declaration_str(self):
		return '\tvoid CopyData(const %s& stArgsData);\n' % self.name

	def copy_data_definition_str(self):
		result = 'void %s::CopyData(const %s& stArgsData)\n' % (self.name, self.name)
		result += '{\n'
		result += ''.join([f.copy_data_str() for f in self.ordered_fields])
		result += '}\n\n'
		return result

	def destruct_declaration_str(self):
		return '\t~%s();\n' % self.name

	def destruct_definition_str(self):
		result = '%s::~%s()\n' % (self.name, self.name)
		result += '{\n'
		result += ''.join([f.destruct_str() for f in self.ordered_fields])
		result += '}\n\n'
		return result

	def createinit_declaration_str(self):
		return '\tint CreateInit();\n'

	def createinit_definition_str(self):
		result = 'int %s::CreateInit()\n' % self.name
		result += '{\n'
		result += ''.join([f.createinit_str() for f in self.ordered_fields])
		result += '\treturn 0;\n'
		result += '}\n\n'
		return result

	def resumeinit_declaration_str(self):
		return '\tint ResumeInit();\n'

	def resumeinit_definition_str(self):
		result = 'int %s::ResumeInit()\n' % self.name
		result += '{\n'
		result += ''.join([f.resumeinit_str() for f in self.ordered_fields])
		result += '\treturn 0;\n'
		result += '}\n\n'
		return result

	def init_declaration_str(self):
		return '\tvoid Init();\n'

	def init_definition_str(self):
		result = 'void %s::Init()\n' % self.name
		result += '{\n'
		result += ''.join([f.init_str() for f in self.ordered_fields])
		result += '}\n\n'
		return result

	def get_which_declaration_str(self):
		if len(self.oneofs) > 0:
			return ''.join([f.get_which_declaration_str() for f in self.ordered_fields])
		else:
			return ''

	def get_which_definition_str(self):
		if len(self.oneofs) > 0:
			return ''.join([f.get_which_definition_str() for f in self.ordered_fields])
		else:
			return ''

	def from_pb_declaration_str(self):
		return '    bool FromPb(const %s& cc);\n' % self.cctype

	def from_pb_definition_str(self):
		result = 'bool %s::FromPb(const %s& cc)\n' % (self.name, self.cctype)
		result += '{\n'
		result += ''.join([f.from_pb_str() for f in self.ordered_fields])
		result += '    return true;\n'
		result += '}\n\n'
		return result

	def to_pb_declaration_str(self):
		result = '    void ToPb(%s* cc) const;\n' % self.cctype
		result +='    %s ToPb() const;\n' % self.cctype
		return result

	def to_pb_definition_str(self):
		result = 'void %s::ToPb(%s* cc) const\n' % (self.name, self.cctype)
		result += '{\n'
		result += ''.join([f.to_pb_str() for f in self.ordered_fields])
		result += '    return;\n'
		result += '}\n\n'
		result += '%s %s::ToPb() const\n' % (self.cctype, self.name)
		result += '{\n'
		result += '    %s cc;\n' % self.cctype
		result += '    ToPb(&cc);\n'
		result += '    return cc;\n'
		result += '}\n\n'
		return result

	def short_debug_string_declaration_str(self):
		return '    std::string ShortDebugString() const;\n'

	def short_debug_string_definition_str(self):
		result = 'std::string %s::ShortDebugString() const\n' % self.name
		result += '{\n'
		result += '    std::stringstream ss;\n'
		result += '    ss << "{";\n'
		for i, f in enumerate(self.ordered_fields):
			result += f.short_debug_string_str()
			if i < (len(self.ordered_fields) - 1) and f.rules != 'ONEOF' and f.rules != 'REPEATED':
				result += ' << ", ";\n'
			else:
				result += ';\n'
		result += '    ss << "}";\n'
		result += '    return ss.str();\n'
		result += '}\n\n'
		return result

	def types(self):
		return ''.join([f.types() for f in self.fields])

	def get_initializer(self, null_init):
		if not self.ordered_fields:
			return '{0}'

		parts = []
		for field in self.ordered_fields:
			parts.append(field.get_initializer(null_init))
		return '{' + ', '.join(parts) + '}'

	def default_decl(self, declaration_only=False):
		result = ""
		for field in self.fields:
			default = field.default_decl(declaration_only)
			if default is not None:
				result += default + '\n'
		return result

	def count_required_fields(self):
		'''Returns number of required fields inside this message'''
		count = 0
		for f in self.fields:
			if not isinstance(f, OneOf):
				if f.rules == 'REQUIRED':
					count += 1
		return count

	def count_all_fields(self):
		count = 0
		for f in self.fields:
			if isinstance(f, OneOf):
				count += len(f.fields)
			else:
				count += 1
		return count

	def fields_declaration(self):
		result = 'extern const pb_field_t %s_fields[%d];' % (self.name, self.count_all_fields() + 1)
		return result

	def fields_definition(self):
		result = 'const pb_field_t %s_fields[%d] = {\n' % (self.name, self.count_all_fields() + 1)

		prev = None
		for field in self.ordered_fields:
			result += field.pb_field_t(prev)
			result += ',\n'
			prev = field.get_last_field_name()

		result += '    PB_LAST_FIELD\n};'
		return result

	def encoded_size(self, dependencies):
		'''Return the maximum size that this message can take when encoded.
		If the size cannot be determined, returns None.
		'''
		size = EncodedSize(0)
		for field in self.fields:
			fsize = field.encoded_size(dependencies)
			if fsize is None:
				return None
			size += fsize

		return size


# ---------------------------------------------------------------------------
#                    Processing of entire .proto files
# ---------------------------------------------------------------------------

def iterate_messages(desc, names=Names()):
	'''Recursively find all messages. For each, yield name, DescriptorProto.'''
	if hasattr(desc, 'message_type'):
		submsgs = desc.message_type
	else:
		submsgs = desc.nested_type

	for submsg in submsgs:
		sub_names = names + submsg.name
		yield sub_names, submsg

		for x in iterate_messages(submsg, sub_names):
			yield x


def iterate_extensions(desc, names=Names()):
	'''Recursively find all extensions.
	For each, yield name, FieldDescriptorProto.
	'''
	for extension in desc.extension:
		yield names, extension

	for subname, subdesc in iterate_messages(desc, names):
		for extension in subdesc.extension:
			yield subname, extension


def toposort2(data):
	'''Topological sort.
	From http://code.activestate.com/recipes/577413-topological-sort/
	This function is under the MIT license.
	'''
	for k, v in list(data.items()):
		v.discard(k)  # Ignore self dependencies
	extra_items_in_deps = reduce(set.union, list(data.values()), set()) - set(data.keys())
	data.update(dict([(item, set()) for item in extra_items_in_deps]))
	while True:
		ordered = set(item for item, dep in list(data.items()) if not dep)
		if not ordered:
			break
		for item in sorted(ordered):
			yield item
		data = dict([(item, (dep - ordered)) for item, dep in list(data.items())
					 if item not in ordered])
	assert not data, "A cyclic dependency exists amongst %r" % data


def sort_dependencies(messages):
	'''Sort a list of Messages based on dependencies.'''
	dependencies = {}
	message_by_name = {}
	for message in messages:
		dependencies[str(message.name)] = set(message.get_dependencies())
		message_by_name[str(message.name)] = message

	for msgname in toposort2(dependencies):
		if msgname in message_by_name:
			yield message_by_name[msgname]


def make_identifier(headername):
	'''Make #ifndef identifier that contains uppercase A-Z and digits 0-9'''
	result = ""
	for c in headername.upper():
		if c.isalnum():
			result += c
		else:
			result += '_'
	return result


class ProtoFile:
	def __init__(self, fdesc, file_options, options):
		'''Takes a FileDescriptorProto and parses it.'''
		self.fdesc = fdesc
		self.file_options = file_options
		self.options = options
		self.dependencies = {}
		self.parse()
		# change-begin
		self.include_files = file_options.include_files.split()
		# change-end
		# Some of types used in this file probably come from the file itself.
		# Thus it has implicit dependency on itself.
		self.add_dependency(self)

	def parse(self):
		self.enums = []
		self.messages = []
		self.extensions = []

		if self.fdesc.package:
			base_name = Names(self.fdesc.package.split('.'))
		else:
			base_name = Names()

		for enum in self.fdesc.enum_type:
			enum_options = get_nanopb_suboptions(enum, self.file_options, base_name + enum.name)
			if enum_options.skip_message:
				continue
			self.enums.append(Enum(base_name, enum, enum_options))

		for names, message in iterate_messages(self.fdesc, base_name):
			message_options = get_nanopb_suboptions(message, self.file_options, names)

			# sys.stderr.write("type_name:%s\n" % (names.namespace_str()))

			if message_options.skip_message:
				continue

			msg = Message(names, message, message_options, names.namespace_str(), self.options)
			self.messages.append(msg)
			msg_name = '%s' % names
			allMessage[msg_name] = msg
			for enum in message.enum_type:
				enum_options = get_nanopb_suboptions(enum, message_options, names + enum.name)
				self.enums.append(Enum(names, enum, enum_options))

		for names, extension in iterate_extensions(self.fdesc, base_name):
			field_options = get_nanopb_suboptions(extension, self.file_options, names + extension.name)
			if field_options.type != nanopb_pb2.FT_IGNORE:
				self.extensions.append(ExtensionField(names, extension, field_options))

	def add_dependency(self, other):
		for enum in other.enums:
			self.dependencies[str(enum.names)] = enum

		for msg in other.messages:
			self.dependencies[str(msg.name)] = msg

		# Fix field default values where enum short names are used.
		for enum in other.enums:
			if not enum.options.long_names:
				for message in self.messages:
					for field in message.fields:
						if id(field.default) in enum.value_longnames_adress_set:
							idx = enum.value_longnames.index(field.default)
							field.default = enum.values[idx][0]

		# Fix field data types where enums have negative values.
		for enum in other.enums:
			if not enum.has_negative():
				for message in self.messages:
					for field in message.fields:
						if field.pbtype == 'ENUM' and field.ctype == enum.names:
							field.pbtype = 'UENUM'

	def generate_field_map(self, options):
		result = '''
#include <map>
struct pb_field_map
{
	struct detail_t
	{
		const pb_field_t* pb_field_ptr;
		size_t pb_size;
	};
	pb_field_map()
	{
'''

		for msg in self.messages:
			result += '        m_DataMap["%s"] = { &%s_fields[0], sizeof(_%s) };\n' % (msg.name, msg.name, msg.name)

		result += '''
	}
	detail_t* get(const std::string& field_name)
	{
		std::map<std::string, detail_t>::iterator it = m_DataMap.find(field_name);
		if(it != m_DataMap.end())
			return &(it->second);
		return NULL;
	}

	std::map<std::string, detail_t> m_DataMap;
}g_pb_field_map;

size_t get_nanopb_type_size(const char* type_name)
{
	pb_field_map::detail_t* pFind = g_pb_field_map.get(type_name);
	if (pFind)
		return pFind->pb_size;
	return 0;
}

const pb_field_t* get_nanopb_field_t(const char* type_name)
{
	pb_field_map::detail_t* pFind = g_pb_field_map.get(type_name);
	if (pFind)
		return pFind->pb_field_ptr;
	return 0;
}


'''
		return result

	def generate_header(self, includes, headername, options):
		'''Generate content for a header file.
		Generates strings, which should be concatenated and stored to file.
		'''

		yield '/* Automatically generated nanopb header */\n'
		if options.notimestamp:
			yield '/* Generated by %s */\n\n' % (nanopb_version)
		else:
			yield '/* Generated by %s at %s. */\n\n' % (nanopb_version, time.asctime())

		if self.fdesc.package:
			symbol = make_identifier(self.fdesc.package + '_' + headername)
		else:
			symbol = make_identifier(headername)
		yield '#ifndef PB_%s_INCLUDED\n' % symbol
		yield '#define PB_%s_INCLUDED\n' % symbol

		yield '#include <string>\n'
		yield '#include <vector>\n'
		yield '#include <unordered_map>\n'
		yield '#include <map>\n'
		yield '#include <NFComm/NFCore/NFHash.hpp>\n'


		for incfile in nfshmHeaderFileList:
			yield "%s\n" % incfile

		try:
			yield options.libformat % ('pb.h')
		except TypeError:
			# no %s specified - use whatever was passed in as options.libformat
			yield options.libformat
		yield '\n'

		cc_headername = '%s.pb.h' % headername.split('.')[0]
		yield '#include "%s"\n' % cc_headername

		for incfile in includes:
			noext = os.path.splitext(incfile)[0]
			yield options.genformat % (noext + options.extension + options.header_extension)
			yield '\n'

		# change-begin
		for custom_inc_file in self.include_files:
			yield '#include %s' % (custom_inc_file,)
			yield '\n'
		# change-end

		yield '\n/* @@protoc_insertion_point(includes) */\n'

		yield '#if PB_PROTO_HEADER_VERSION != 30\n'
		yield '#error Regenerate this file with the current version of nanopb generator.\n'
		yield '#endif\n'
		yield '\n'

		if not options.source_extension == '.cc':
			yield '#ifdef __cplusplus\n'
			yield 'extern "C" {\n'
			yield '#endif\n\n'

		if self.enums:
			yield '/* Enum definitions */\n'
			for enum in self.enums:
				yield str(enum) + '\n\n'

		if self.messages:
			yield '/* Struct definitions */\n'
			for msg in sort_dependencies(self.messages):
				yield msg.types()
				yield str(msg) + '\n\n'

		if self.extensions:
			yield '/* Extensions */\n'
			for extension in self.extensions:
				yield extension.extension_decl()
			yield '\n'

		if self.messages:
			#yield '/* Default values for struct fields */\n'
			#for msg in self.messages:
			#	yield msg.default_decl(True)
			#yield '\n'

			#yield '/* Initializer values for message structs */\n'
			#for msg in self.messages:
			#    identifier = '%s_init_default' % msg.name
			#    yield '#define %-40s %s\n' % (identifier, msg.get_initializer(False))
			#for msg in self.messages:
			#    identifier = '%s_init_zero' % msg.name
			#    yield '#define %-40s %s\n' % (identifier, msg.get_initializer(True))
			#yield '\n'

			yield '/* Field tags (for use in manual encoding/decoding) */\n'
			for msg in sort_dependencies(self.messages):
				for field in msg.fields:
					yield field.tags()
			for extension in self.extensions:
				yield extension.tags()
			yield '\n'

			yield '/* Struct field encoding specification for nanopb */\n'
			#for msg in self.messages:
			#    yield msg.fields_declaration() + '\n'
			yield '\n'

			yield '/* Maximum encoded size of messages (where known) */\n'
			for msg in self.messages:
				msize = msg.encoded_size(self.dependencies)
				identifier = '%s_size' % msg.name
				if msize is not None:
					try:
						nSize = int(str(msize))
						yield '#define %-40s %d\n' % (identifier, nSize)
					except ValueError as e:
						yield '#define %-40s (sizeof(%s) * 2)\n' % (identifier, str(identifier)[:-5])
				else:
					yield '/* %s depends on runtime parameters */\n' % identifier
			yield '\n'

			yield '/* Message IDs (where set with "msgid" option) */\n'

			yield '#ifdef PB_MSGID\n'
			for msg in self.messages:
				if hasattr(msg, 'msgid'):
					yield '#define PB_MSG_%d %s\n' % (msg.msgid, msg.name)
			yield '\n'

			symbol = make_identifier(headername.split('.')[0])
			yield '#define %s_MESSAGES \\\n' % symbol

			for msg in self.messages:
				m = "-1"
				msize = msg.encoded_size(self.dependencies)
				if msize is not None:
					m = msize
				if hasattr(msg, 'msgid'):
					yield '\tPB_MSG(%d,%s,%s) \\\n' % (msg.msgid, m, msg.name)
			yield '\n'

			for msg in self.messages:
				if hasattr(msg, 'msgid'):
					yield '#define %s_msgid %d\n' % (msg.name, msg.msgid)
			yield '\n'

			yield '#endif\n\n'
		if not options.source_extension == '.cc':
			yield '#ifdef __cplusplus\n'
			yield '} /* extern "C" */\n'
			yield '#endif\n'

		if options.export_field_map:
			yield 'size_t get_nanopb_type_size(const char* type_name);\n'
			yield 'const pb_field_t* get_nanopb_field_t(const char* type_name);\n'

		# End of header
		yield '/* @@protoc_insertion_point(eof) */\n'
		yield '\n#endif\n'

	def generate_source(self, headername, options):
		'''Generate content for a source file.'''

		yield '/* Automatically generated nanopb constant definitions */\n'
		if options.notimestamp:
			yield '/* Generated by %s */\n\n' % (nanopb_version)
		else:
			yield '/* Generated by %s at %s. */\n\n' % (nanopb_version, time.asctime())

		yield '#include <sstream>\n'
		yield '#include "NFComm/NFObjCommon/NFShmMgr.h"\n'

		yield options.genformat % (headername)
		yield '\n'
		yield '/* @@protoc_insertion_point(includes) */\n'

		yield '#if PB_PROTO_HEADER_VERSION != 30\n'
		yield '#error Regenerate this file with the current version of nanopb generator.\n'
		yield '#endif\n'
		yield '\n'

		for msg in self.messages:
			#yield msg.default_decl(False)
			if options.source_extension == '.cc' and msg.all_fields_static:
				# if options.source_extension == '.cc':
				yield msg.construct_definition_str()
				yield msg.destruct_definition_str()
				yield msg.copy_construct_definition_str()
				yield msg.copy_operator_definition_str()
				yield msg.copy_data_definition_str()
				yield msg.createinit_definition_str()
				yield msg.resumeinit_definition_str()
				yield msg.init_definition_str()
				yield msg.from_pb_definition_str()
				yield msg.to_pb_definition_str()
				yield msg.short_debug_string_definition_str()
				yield msg.get_which_definition_str()

		yield '\n\n'

		#for msg in self.messages:
		#    yield msg.fields_definition() + '\n\n'

		for ext in self.extensions:
			yield ext.extension_def() + '\n'

		for enum in self.enums:
			yield enum.enum_to_string_definition() + '\n'

		if options.export_field_map:
			yield '\n/* export_field_map */\n' + self.generate_field_map(options)

		# Add checks for numeric limits
		if self.messages:
			largest_msg = max(self.messages, key=lambda m: m.count_required_fields())
			largest_count = largest_msg.count_required_fields()
			if largest_count > 64:
				yield '\n/* Check that missing required fields will be properly detected */\n'
				yield '#if PB_MAX_REQUIRED_FIELDS < %d\n' % largest_count
				yield '#error Properly detecting missing required fields in %s requires \\\n' % largest_msg.name
				yield '       setting PB_MAX_REQUIRED_FIELDS to %d or more.\n' % largest_count
				yield '#endif\n'

		max_field = FieldMaxSize()
		checks_msgnames = []
		for msg in self.messages:
			checks_msgnames.append(msg.name)
			for field in msg.fields:
				max_field.extend(field.largest_field_value())
		for field in self.extensions:
			max_field.extend(field.largest_field_value())

		worst = max_field.worst
		worst_field = max_field.worst_field
		checks = max_field.checks

		if worst > 255 or checks:
			yield '\n/* Check that field information fits in pb_field_t */\n'

			if worst > 65535 or checks:
				yield '#if !defined(PB_FIELD_32BIT)\n'
				if worst > 65535:
					yield '#error Field descriptor for %s is too large. Define PB_FIELD_32BIT to fix this.\n' % worst_field
				else:
					assertion = ' && '.join(str(c) + ' < 65536' for c in checks)
					msgs = '_'.join(str(n) for n in checks_msgnames)
					yield '/* If you get an error here, it means that you need to define PB_FIELD_32BIT\n'
					yield ' * compile-time option. You can do that in pb.h or on compiler command line.\n'
					yield ' * \n'
					yield ' * The reason you need to do this is that some of your messages contain tag\n'
					yield ' * numbers or field sizes that are larger than what can fit in 8 or 16 bit\n'
					yield ' * field descriptors.\n'
					yield ' */\n'
					yield 'PB_STATIC_ASSERT((%s), YOU_MUST_DEFINE_PB_FIELD_32BIT_FOR_MESSAGES_%s)\n' % (assertion, msgs)
				yield '#endif\n\n'

			if worst < 65536:
				yield '#if !defined(PB_FIELD_16BIT) && !defined(PB_FIELD_32BIT)\n'
				if worst > 255:
					yield '#error Field descriptor for %s is too large. Define PB_FIELD_16BIT to fix this.\n' % worst_field
				else:
					assertion = ' && '.join(str(c) + ' < 256' for c in checks)
					msgs = '_'.join(str(n) for n in checks_msgnames)
					yield '/* If you get an error here, it means that you need to define PB_FIELD_16BIT\n'
					yield ' * compile-time option. You can do that in pb.h or on compiler command line.\n'
					yield ' * \n'
					yield ' * The reason you need to do this is that some of your messages contain tag\n'
					yield ' * numbers or field sizes that are larger than what can fit in the default\n'
					yield ' * 8 bit descriptors.\n'
					yield ' */\n'
					yield 'PB_STATIC_ASSERT((%s), YOU_MUST_DEFINE_PB_FIELD_16BIT_FOR_MESSAGES_%s)\n' % (assertion, msgs)
				yield '#endif\n\n'

		# Add check for sizeof(double)
		has_double = False
		for msg in self.messages:
			for field in msg.fields:
				if field.ctype == 'double':
					has_double = True

		if has_double:
			yield '\n'
			yield '/* On some platforms (such as AVR), double is really float.\n'
			yield ' * These are not directly supported by nanopb, but see example_avr_double.\n'
			yield ' * To get rid of this error, remove any double fields from your .proto.\n'
			yield ' */\n'
			yield 'PB_STATIC_ASSERT(sizeof(double) == 8, DOUBLE_MUST_BE_8_BYTES)\n'

		yield '\n'
		yield '/* @@protoc_insertion_point(eof) */\n'


# ---------------------------------------------------------------------------
#                    Options parsing for the .proto files
# ---------------------------------------------------------------------------

from fnmatch import fnmatch


def read_options_file(infile):
	'''Parse a separate options file to list:
		[(namemask, options), ...]
	'''
	results = []
	data = infile.read()
	data = re.sub('/\*.*?\*/', '', data, flags=re.MULTILINE)
	data = re.sub('//.*?$', '', data, flags=re.MULTILINE)
	data = re.sub('#.*?$', '', data, flags=re.MULTILINE)
	for i, line in enumerate(data.split('\n')):
		line = line.strip()
		if not line:
			continue

		parts = line.split(None, 1)

		if len(parts) < 2:
			sys.stderr.write("%s:%d: " % (infile.name, i + 1) +
							 "Option lines should have space between field name and options. " +
							 "Skipping line: '%s'\n" % line)
			continue

		opts = nanopb_pb2.NanoPBOptions()

		try:
			text_format.Merge(parts[1], opts)
		except Exception as e:
			sys.stderr.write("%s:%d: " % (infile.name, i + 1) +
							 "Unparseable option line: '%s'. " % line +
							 "Error: %s\n" % str(e))
			continue
		results.append((parts[0], opts))

	return results


class Globals:
	'''Ugly global variables, should find a good way to pass these.'''
	verbose_options = False
	separate_options = []
	matched_namemasks = set()


def get_nanopb_suboptions(subdesc, options, name):
	'''Get copy of options, and merge information from subdesc.'''
	new_options = nanopb_pb2.NanoPBOptions()
	new_options.CopyFrom(options)

	if hasattr(subdesc, 'syntax') and subdesc.syntax == "proto3":
		new_options.proto3 = True

	# Handle options defined in a separate file
	dotname = '.'.join(name.parts)
	for namemask, options in Globals.separate_options:
		if fnmatch(dotname, namemask):
			Globals.matched_namemasks.add(namemask)
			new_options.MergeFrom(options)

	# Handle options defined in .proto
	if isinstance(subdesc.options, descriptor.FieldOptions):
		ext_type = nanopb_pb2.nanopb
	elif isinstance(subdesc.options, descriptor.FileOptions):
		ext_type = nanopb_pb2.nanopb_fileopt
	elif isinstance(subdesc.options, descriptor.MessageOptions):
		ext_type = nanopb_pb2.nanopb_msgopt
	elif isinstance(subdesc.options, descriptor.EnumOptions):
		ext_type = nanopb_pb2.nanopb_enumopt
	else:
		raise Exception("Unknown options type")

	if subdesc.options.HasExtension(ext_type):
		ext = subdesc.options.Extensions[ext_type]
		new_options.MergeFrom(ext)

	if Globals.verbose_options:
		sys.stderr.write("Options for " + dotname + ": ")
		sys.stderr.write(text_format.MessageToString(new_options) + "\n")

	return new_options


# ---------------------------------------------------------------------------
#                         Command line interface
# ---------------------------------------------------------------------------

import sys
import os.path
from optparse import OptionParser

optparser = OptionParser(
	usage="Usage: nanopb_generator.py [options] file.pb ...",
	epilog="Compile file.pb from file.proto by: 'protoc -ofile.pb file.proto'. " +
		   "Output will be written to file.pb.h and file.pb.c.")
optparser.add_option("-x", dest="exclude", metavar="FILE", action="append", default=[],
					 help="Exclude file from generated #include list.")
optparser.add_option("-e", "--extension", dest="extension", metavar="EXTENSION", default=".nanopb",
					 help="Set extension to use instead of '.pb' for generated files. [default: %default]")
optparser.add_option("-H", "--header-extension", dest="header_extension", metavar="EXTENSION", default=".h",
					 help="Set extension to use for generated header files. [default: %default]")
optparser.add_option("-S", "--source-extension", dest="source_extension", metavar="EXTENSION", default=".cc",
					 help="Set extension to use for generated source files. [default: %default]")
optparser.add_option("-f", "--options-file", dest="options_file", metavar="FILE", default="%s.options",
					 help="Set name of a separate generator options file.")
optparser.add_option("-I", "--options-path", dest="options_path", metavar="DIR",
					 action="append", default=[],
					 help="Search for .options files additionally in this path")
optparser.add_option("-D", "--output-dir", dest="output_dir",
					 metavar="OUTPUTDIR", default=None,
					 help="Output directory of .pb.h and .pb.c files")
optparser.add_option("-Q", "--generated-include-format", dest="genformat",
					 metavar="FORMAT", default='#include "%s"\n',
					 help="Set format string to use for including other .pb.h files. [default: %default]")
optparser.add_option("-L", "--library-include-format", dest="libformat",
					 metavar="FORMAT", default='#include <%s>\n',
					 help="Set format string to use for including the nanopb pb.h header. [default: %default]")
optparser.add_option("-T", "--no-timestamp", dest="notimestamp", action="store_true", default=True,
					 help="Don't add timestamp to .pb.h and .pb.c preambles")
optparser.add_option("-q", "--quiet", dest="quiet", action="store_true", default=False,
					 help="Don't print anything except errors.")
optparser.add_option("-v", "--verbose", dest="verbose", action="store_true", default=False,
					 help="Print more information.")
optparser.add_option("-s", dest="settings", metavar="OPTION:VALUE", action="append", default=[],
					 help="Set generator option (max_size, max_count etc.).")
optparser.add_option("--export_field_map", dest="export_field_map", action="store_true", default=False,
					 help="exprot field map.")


def parse_file(filename, fdesc, options):
	'''Parse a single file. Returns a ProtoFile instance.'''
	toplevel_options = nanopb_pb2.NanoPBOptions()
	for s in options.settings:
		text_format.Merge(s, toplevel_options)

	if not fdesc:
		data = open(filename, 'rb').read()
		fdesc = descriptor.FileDescriptorSet.FromString(data).file[0]

	# Check if there is a separate .options file
	had_abspath = False
	try:
		optfilename = options.options_file % os.path.splitext(filename)[0]
	except TypeError:
		# No %s specified, use the filename as-is
		optfilename = options.options_file
		had_abspath = True

	paths = ['.'] + options.options_path
	for p in paths:
		if os.path.isfile(os.path.join(p, optfilename)):
			optfilename = os.path.join(p, optfilename)
			if options.verbose:
				sys.stderr.write('Reading options from ' + optfilename + '\n')
			Globals.separate_options = read_options_file(open(optfilename, "rU"))
			break
	else:
		# If we are given a full filename and it does not exist, give an error.
		# However, don't give error when we automatically look for .options file
		# with the same name as .proto.
		if options.verbose or had_abspath:
			sys.stderr.write('Options file not found: ' + optfilename + '\n')
		Globals.separate_options = []

	Globals.matched_namemasks = set()

	# Parse the file
	file_options = get_nanopb_suboptions(fdesc, toplevel_options, Names([filename]))
	f = ProtoFile(fdesc, file_options, options)
	f.optfilename = optfilename

	return f


def process_file(filename, fdesc, options, other_files={}):
	'''Process a single file.
	filename: The full path to the .proto or .pb source file, as string.
	fdesc: The loaded FileDescriptorSet, or None to read from the input file.
	options: Command line options as they come from OptionsParser.

	Returns a dict:
		{'headername': Name of header file,
		 'headerdata': Data for the .h header file,
		 'sourcename': Name of the source code file,
		 'sourcedata': Data for the .c source code file
		}
	'''
	global nfshmHeaderFileList
	global nfshmHeaderFileMap
	nfshmHeaderFileMap = {}
	nfshmHeaderFileList = []
	f = parse_file(filename, fdesc, options)

	# Provide dependencies if available
	for dep in f.fdesc.dependency:
		if dep in other_files:
			f.add_dependency(other_files[dep])

	# Decide the file names
	noext = os.path.splitext(filename)[0]
	headername = noext + options.extension + options.header_extension
	sourcename = noext + options.extension + options.source_extension
	headerbasename = os.path.basename(headername)

	# List of .proto files that should not be included in the C header file
	# even if they are mentioned in the source .proto.
	excludes = ['nanopb.proto', 'google/protobuf/descriptor.proto'] + options.exclude
	includes = [d for d in f.fdesc.dependency if d not in excludes]

	headerdata = ''.join(f.generate_header(includes, headerbasename, options))
	sourcedata = ''.join(f.generate_source(headerbasename, options))

	# Check if there were any lines in .options that did not match a member
	unmatched = [n for n, o in Globals.separate_options if n not in Globals.matched_namemasks]
	if unmatched and not options.quiet:
		sys.stderr.write("Following patterns in " + f.optfilename + " did not match any fields: "
						 + ', '.join(unmatched) + "\n")
		if not Globals.verbose_options:
			sys.stderr.write("Use  protoc --nanopb-out=-v:.   to see a list of the field names.\n")

	return {'headername': headername, 'headerdata': headerdata,
			'sourcename': sourcename, 'sourcedata': sourcedata}


def main_cli():
	'''Main function when invoked directly from the command line.'''

	options, filenames = optparser.parse_args()

	if not filenames:
		optparser.print_help()
		sys.exit(1)

	if options.quiet:
		options.verbose = False

	if options.output_dir and not os.path.exists(options.output_dir):
		optparser.print_help()
		sys.stderr.write("\noutput_dir does not exist: %s\n" % options.output_dir)
		sys.exit(1)

	if options.verbose:
		sys.stderr.write('Google Python protobuf library imported from %s, version %s\n'
						 % (google.protobuf.__file__, google.protobuf.__version__))

	Globals.verbose_options = options.verbose
	for filename in filenames:
		results = process_file(filename, None, options)

		base_dir = options.output_dir or ''
		to_write = [
			(os.path.join(base_dir, results['headername']), results['headerdata']),
			(os.path.join(base_dir, results['sourcename']), results['sourcedata']),
		]

		if not options.quiet:
			paths = " and ".join([x[0] for x in to_write])
			sys.stderr.write("Writing to %s\n" % paths)

		for path, data in to_write:
			with open(path, 'w') as f:
				f.write(data)


def main_plugin():
	'''Main function when invoked as a protoc plugin.'''

	import io, sys
	if sys.platform == "win32":
		import os, msvcrt
		# Set stdin and stdout to binary mode
		msvcrt.setmode(sys.stdin.fileno(), os.O_BINARY)
		msvcrt.setmode(sys.stdout.fileno(), os.O_BINARY)

	data = io.open(sys.stdin.fileno(), "rb").read()

	request = plugin_pb2.CodeGeneratorRequest.FromString(data)

	try:
		# Versions of Python prior to 2.7.3 do not support unicode
		# input to shlex.split(). Try to convert to str if possible.
		params = str(request.parameter)
	except UnicodeEncodeError:
		params = request.parameter

	import shlex
	args = shlex.split(params)
	options, dummy = optparser.parse_args(args)

	Globals.verbose_options = options.verbose

	if options.verbose:
		sys.stderr.write('Google Python protobuf library imported from %s, version %s\n'
						 % (google.protobuf.__file__, google.protobuf.__version__))

	response = plugin_pb2.CodeGeneratorResponse()

	# Google's protoc does not currently indicate the full path of proto files.
	# Instead always add the main file path to the search dirs, that works for
	# the common case.
	import os.path
	options.options_path.append(os.path.dirname(request.file_to_generate[0]))

	# Process any include files first, in order to have them
	# available as dependencies
	other_files = {}
	for fdesc in request.proto_file:
		other_files[fdesc.name] = parse_file(fdesc.name, fdesc, options)

	for filename in request.file_to_generate:
		for fdesc in request.proto_file:
			if fdesc.name == filename:
				results = process_file(filename, fdesc, options, other_files)

				f = response.file.add()
				f.name = results['headername']
				f.content = results['headerdata']

				f = response.file.add()
				f.name = results['sourcename']
				f.content = results['sourcedata']

	io.open(sys.stdout.fileno(), "wb").write(response.SerializeToString())


if __name__ == '__main__':
	# Check if we are running as a plugin under protoc
	if 'protoc-gen-' in sys.argv[0] or '--protoc-plugin' in sys.argv:
		main_plugin()
	else:
		main_cli()
