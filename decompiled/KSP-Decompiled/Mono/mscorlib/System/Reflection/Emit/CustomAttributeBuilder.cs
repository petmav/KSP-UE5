using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;

namespace System.Reflection.Emit;

[StructLayout(LayoutKind.Sequential)]
[ClassInterface(ClassInterfaceType.None)]
[ComVisible(true)]
[ComDefaultInterface(typeof(_CustomAttributeBuilder))]
public class CustomAttributeBuilder : _CustomAttributeBuilder
{
	internal struct CustomAttributeInfo
	{
		public ConstructorInfo ctor;

		public object[] ctorArgs;

		public string[] namedParamNames;

		public object[] namedParamValues;
	}

	private ConstructorInfo ctor;

	private byte[] data;

	private object[] args;

	private PropertyInfo[] namedProperties;

	private object[] propertyValues;

	private FieldInfo[] namedFields;

	private object[] fieldValues;

	internal ConstructorInfo Ctor => ctor;

	internal byte[] Data => data;

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern byte[] GetBlob(Assembly asmb, ConstructorInfo con, object[] constructorArgs, PropertyInfo[] namedProperties, object[] propertyValues, FieldInfo[] namedFields, object[] fieldValues);

	internal object Invoke()
	{
		object obj = ctor.Invoke(args);
		for (int i = 0; i < namedFields.Length; i++)
		{
			namedFields[i].SetValue(obj, fieldValues[i]);
		}
		for (int j = 0; j < namedProperties.Length; j++)
		{
			namedProperties[j].SetValue(obj, propertyValues[j]);
		}
		return obj;
	}

	internal CustomAttributeBuilder(ConstructorInfo con, byte[] binaryAttribute)
	{
		if (con == null)
		{
			throw new ArgumentNullException("con");
		}
		if (binaryAttribute == null)
		{
			throw new ArgumentNullException("binaryAttribute");
		}
		ctor = con;
		data = (byte[])binaryAttribute.Clone();
	}

	public CustomAttributeBuilder(ConstructorInfo con, object[] constructorArgs)
	{
		Initialize(con, constructorArgs, new PropertyInfo[0], new object[0], new FieldInfo[0], new object[0]);
	}

	public CustomAttributeBuilder(ConstructorInfo con, object[] constructorArgs, FieldInfo[] namedFields, object[] fieldValues)
	{
		Initialize(con, constructorArgs, new PropertyInfo[0], new object[0], namedFields, fieldValues);
	}

	public CustomAttributeBuilder(ConstructorInfo con, object[] constructorArgs, PropertyInfo[] namedProperties, object[] propertyValues)
	{
		Initialize(con, constructorArgs, namedProperties, propertyValues, new FieldInfo[0], new object[0]);
	}

	public CustomAttributeBuilder(ConstructorInfo con, object[] constructorArgs, PropertyInfo[] namedProperties, object[] propertyValues, FieldInfo[] namedFields, object[] fieldValues)
	{
		Initialize(con, constructorArgs, namedProperties, propertyValues, namedFields, fieldValues);
	}

	private bool IsValidType(Type t)
	{
		if (t.IsArray && t.GetArrayRank() > 1)
		{
			return false;
		}
		if (t is TypeBuilder && t.IsEnum)
		{
			Enum.GetUnderlyingType(t);
		}
		if (t.IsClass && !t.IsArray && !(t == typeof(object)) && !(t == typeof(Type)) && !(t == typeof(string)) && !(t.Assembly.GetName().Name == "mscorlib"))
		{
			return false;
		}
		if (t.IsValueType && !t.IsPrimitive && !t.IsEnum && (!(t.Assembly is AssemblyBuilder) || !(t.Assembly.GetName().Name == "mscorlib")))
		{
			return false;
		}
		return true;
	}

	private bool IsValidParam(object o, Type paramType)
	{
		Type type = o.GetType();
		if (!IsValidType(type))
		{
			return false;
		}
		if (paramType == typeof(object))
		{
			if (type.IsArray && type.GetArrayRank() == 1)
			{
				return IsValidType(type.GetElementType());
			}
			if (!type.IsPrimitive && !typeof(Type).IsAssignableFrom(type) && type != typeof(string) && !type.IsEnum)
			{
				return false;
			}
		}
		return true;
	}

	private static bool IsValidValue(Type type, object value)
	{
		if (type.IsValueType && value == null)
		{
			return false;
		}
		if (type.IsArray && type.GetElementType().IsValueType)
		{
			foreach (object item in (Array)value)
			{
				if (item == null)
				{
					return false;
				}
			}
		}
		return true;
	}

	private void Initialize(ConstructorInfo con, object[] constructorArgs, PropertyInfo[] namedProperties, object[] propertyValues, FieldInfo[] namedFields, object[] fieldValues)
	{
		ctor = con;
		args = constructorArgs;
		this.namedProperties = namedProperties;
		this.propertyValues = propertyValues;
		this.namedFields = namedFields;
		this.fieldValues = fieldValues;
		if (con == null)
		{
			throw new ArgumentNullException("con");
		}
		if (constructorArgs == null)
		{
			throw new ArgumentNullException("constructorArgs");
		}
		if (namedProperties == null)
		{
			throw new ArgumentNullException("namedProperties");
		}
		if (propertyValues == null)
		{
			throw new ArgumentNullException("propertyValues");
		}
		if (namedFields == null)
		{
			throw new ArgumentNullException("namedFields");
		}
		if (fieldValues == null)
		{
			throw new ArgumentNullException("fieldValues");
		}
		if (con.GetParametersCount() != constructorArgs.Length)
		{
			throw new ArgumentException("Parameter count does not match passed in argument value count.");
		}
		if (namedProperties.Length != propertyValues.Length)
		{
			throw new ArgumentException("Array lengths must be the same.", "namedProperties, propertyValues");
		}
		if (namedFields.Length != fieldValues.Length)
		{
			throw new ArgumentException("Array lengths must be the same.", "namedFields, fieldValues");
		}
		if ((con.Attributes & MethodAttributes.Static) == MethodAttributes.Static || (con.Attributes & MethodAttributes.MemberAccessMask) == MethodAttributes.Private)
		{
			throw new ArgumentException("Cannot have private or static constructor.");
		}
		Type declaringType = ctor.DeclaringType;
		int num = 0;
		foreach (FieldInfo fieldInfo in namedFields)
		{
			Type declaringType2 = fieldInfo.DeclaringType;
			if (declaringType != declaringType2 && !declaringType2.IsSubclassOf(declaringType) && !declaringType.IsSubclassOf(declaringType2))
			{
				throw new ArgumentException("Field '" + fieldInfo.Name + "' does not belong to the same class as the constructor");
			}
			if (!IsValidType(fieldInfo.FieldType))
			{
				throw new ArgumentException("Field '" + fieldInfo.Name + "' does not have a valid type.");
			}
			if (!IsValidValue(fieldInfo.FieldType, fieldValues[num]))
			{
				throw new ArgumentException("Field " + fieldInfo.Name + " is not a valid value.");
			}
			if (fieldValues[num] != null && !(fieldInfo.FieldType is TypeBuilder) && !fieldInfo.FieldType.IsEnum && !fieldInfo.FieldType.IsInstanceOfType(fieldValues[num]) && !fieldInfo.FieldType.IsArray)
			{
				throw new ArgumentException("Value of field '" + fieldInfo.Name + "' does not match field type: " + fieldInfo.FieldType);
			}
			num++;
		}
		num = 0;
		foreach (PropertyInfo propertyInfo in namedProperties)
		{
			if (!propertyInfo.CanWrite)
			{
				throw new ArgumentException("Property '" + propertyInfo.Name + "' does not have a setter.");
			}
			Type declaringType3 = propertyInfo.DeclaringType;
			if (declaringType != declaringType3 && !declaringType3.IsSubclassOf(declaringType) && !declaringType.IsSubclassOf(declaringType3))
			{
				throw new ArgumentException("Property '" + propertyInfo.Name + "' does not belong to the same class as the constructor");
			}
			if (!IsValidType(propertyInfo.PropertyType))
			{
				throw new ArgumentException("Property '" + propertyInfo.Name + "' does not have a valid type.");
			}
			if (!IsValidValue(propertyInfo.PropertyType, propertyValues[num]))
			{
				throw new ArgumentException("Property " + propertyInfo.Name + " is not a valid value.");
			}
			if (propertyValues[num] != null && !(propertyInfo.PropertyType is TypeBuilder) && !propertyInfo.PropertyType.IsEnum && !propertyInfo.PropertyType.IsInstanceOfType(propertyValues[num]) && !propertyInfo.PropertyType.IsArray)
			{
				throw new ArgumentException(string.Concat("Value of property '", propertyInfo.Name, "' does not match property type: ", propertyInfo.PropertyType, " -> ", propertyValues[num]));
			}
			num++;
		}
		num = 0;
		ParameterInfo[] parameters = GetParameters(con);
		foreach (ParameterInfo parameterInfo in parameters)
		{
			if (parameterInfo != null)
			{
				Type parameterType = parameterInfo.ParameterType;
				if (!IsValidType(parameterType))
				{
					throw new ArgumentException("Parameter " + num + " does not have a valid type.");
				}
				if (!IsValidValue(parameterType, constructorArgs[num]))
				{
					throw new ArgumentException("Parameter " + num + " is not a valid value.");
				}
				if (constructorArgs[num] != null)
				{
					if (!(parameterType is TypeBuilder) && !parameterType.IsEnum && !parameterType.IsInstanceOfType(constructorArgs[num]) && !parameterType.IsArray)
					{
						throw new ArgumentException(string.Concat("Value of argument ", num, " does not match parameter type: ", parameterType, " -> ", constructorArgs[num]));
					}
					if (!IsValidParam(constructorArgs[num], parameterType))
					{
						throw new ArgumentException(string.Concat("Cannot emit a CustomAttribute with argument of type ", constructorArgs[num].GetType(), "."));
					}
				}
			}
			num++;
		}
		data = GetBlob(declaringType.Assembly, con, constructorArgs, namedProperties, propertyValues, namedFields, fieldValues);
	}

	internal static int decode_len(byte[] data, int pos, out int rpos)
	{
		int num = 0;
		if ((data[pos] & 0x80) == 0)
		{
			num = data[pos++] & 0x7F;
		}
		else if ((data[pos] & 0x40) == 0)
		{
			num = ((data[pos] & 0x3F) << 8) + data[pos + 1];
			pos += 2;
		}
		else
		{
			num = ((data[pos] & 0x1F) << 24) + (data[pos + 1] << 16) + (data[pos + 2] << 8) + data[pos + 3];
			pos += 4;
		}
		rpos = pos;
		return num;
	}

	internal static string string_from_bytes(byte[] data, int pos, int len)
	{
		return Encoding.UTF8.GetString(data, pos, len);
	}

	internal string string_arg()
	{
		int rpos = 2;
		int len = decode_len(data, rpos, out rpos);
		return string_from_bytes(data, rpos, len);
	}

	internal static UnmanagedMarshal get_umarshal(CustomAttributeBuilder customBuilder, bool is_field)
	{
		byte[] array = customBuilder.Data;
		UnmanagedType elemType = (UnmanagedType)80;
		int num = -1;
		int sizeParamIndex = -1;
		bool flag = false;
		string text = null;
		Type typeref = null;
		string cookie = string.Empty;
		int num2 = array[2];
		num2 |= array[3] << 8;
		string fullName = GetParameters(customBuilder.Ctor)[0].ParameterType.FullName;
		int rpos = 6;
		if (fullName == "System.Int16")
		{
			rpos = 4;
		}
		int num3 = array[rpos++];
		num3 |= array[rpos++] << 8;
		for (int i = 0; i < num3; i++)
		{
			rpos++;
			if (array[rpos++] == 85)
			{
				int num4 = decode_len(array, rpos, out rpos);
				string_from_bytes(array, rpos, num4);
				rpos += num4;
			}
			int num5 = decode_len(array, rpos, out rpos);
			string text2 = string_from_bytes(array, rpos, num5);
			rpos += num5;
			switch (text2)
			{
			case "ArraySubType":
				elemType = (UnmanagedType)(array[rpos++] | (array[rpos++] << 8) | (array[rpos++] << 16) | (array[rpos++] << 24));
				break;
			case "SizeConst":
				num = array[rpos++] | (array[rpos++] << 8) | (array[rpos++] << 16) | (array[rpos++] << 24);
				flag = true;
				break;
			case "SafeArraySubType":
				elemType = (UnmanagedType)(array[rpos++] | (array[rpos++] << 8) | (array[rpos++] << 16) | (array[rpos++] << 24));
				break;
			case "IidParameterIndex":
				rpos += 4;
				break;
			case "SafeArrayUserDefinedSubType":
				num5 = decode_len(array, rpos, out rpos);
				string_from_bytes(array, rpos, num5);
				rpos += num5;
				break;
			case "SizeParamIndex":
				sizeParamIndex = array[rpos++] | (array[rpos++] << 8);
				flag = true;
				break;
			case "MarshalType":
				num5 = decode_len(array, rpos, out rpos);
				text = string_from_bytes(array, rpos, num5);
				rpos += num5;
				break;
			case "MarshalTypeRef":
				num5 = decode_len(array, rpos, out rpos);
				text = string_from_bytes(array, rpos, num5);
				typeref = Type.GetType(text);
				rpos += num5;
				break;
			case "MarshalCookie":
				num5 = decode_len(array, rpos, out rpos);
				cookie = string_from_bytes(array, rpos, num5);
				rpos += num5;
				break;
			default:
				throw new Exception("Unknown MarshalAsAttribute field: " + text2);
			}
		}
		switch ((UnmanagedType)num2)
		{
		case UnmanagedType.LPArray:
			if (flag)
			{
				return UnmanagedMarshal.DefineLPArrayInternal(elemType, num, sizeParamIndex);
			}
			return UnmanagedMarshal.DefineLPArray(elemType);
		case UnmanagedType.SafeArray:
			return UnmanagedMarshal.DefineSafeArray(elemType);
		case UnmanagedType.ByValArray:
			if (!is_field)
			{
				throw new ArgumentException("Specified unmanaged type is only valid on fields");
			}
			return UnmanagedMarshal.DefineByValArray(num);
		case UnmanagedType.ByValTStr:
			return UnmanagedMarshal.DefineByValTStr(num);
		case UnmanagedType.CustomMarshaler:
			return UnmanagedMarshal.DefineCustom(typeref, cookie, text, Guid.Empty);
		default:
			return UnmanagedMarshal.DefineUnmanagedMarshal((UnmanagedType)num2);
		}
	}

	private static Type elementTypeToType(int elementType)
	{
		return elementType switch
		{
			2 => typeof(bool), 
			3 => typeof(char), 
			4 => typeof(sbyte), 
			5 => typeof(byte), 
			6 => typeof(short), 
			7 => typeof(ushort), 
			8 => typeof(int), 
			9 => typeof(uint), 
			10 => typeof(long), 
			11 => typeof(ulong), 
			12 => typeof(float), 
			13 => typeof(double), 
			14 => typeof(string), 
			_ => throw new Exception("Unknown element type '" + elementType + "'"), 
		};
	}

	private static object decode_cattr_value(Type t, byte[] data, int pos, out int rpos)
	{
		switch (Type.GetTypeCode(t))
		{
		case TypeCode.String:
		{
			if (data[pos] == byte.MaxValue)
			{
				rpos = pos + 1;
				return null;
			}
			int num2 = decode_len(data, pos, out pos);
			rpos = pos + num2;
			return string_from_bytes(data, pos, num2);
		}
		case TypeCode.Int32:
			rpos = pos + 4;
			return data[pos] + (data[pos + 1] << 8) + (data[pos + 2] << 16) + (data[pos + 3] << 24);
		case TypeCode.Boolean:
			rpos = pos + 1;
			return (data[pos] != 0) ? true : false;
		case TypeCode.Object:
		{
			int num = data[pos];
			pos++;
			if (num >= 2 && num <= 14)
			{
				return decode_cattr_value(elementTypeToType(num), data, pos, out rpos);
			}
			throw new Exception("Subtype '" + num + "' of type object not yet handled in decode_cattr_value");
		}
		default:
			throw new Exception(string.Concat("FIXME: Type ", t, " not yet handled in decode_cattr_value."));
		}
	}

	internal static CustomAttributeInfo decode_cattr(CustomAttributeBuilder customBuilder)
	{
		byte[] array = customBuilder.Data;
		ConstructorInfo constructorInfo = customBuilder.Ctor;
		int num = 0;
		CustomAttributeInfo result = default(CustomAttributeInfo);
		if (array.Length < 2)
		{
			throw new Exception("Custom attr length is only '" + array.Length + "'");
		}
		if (array[0] != 1 || array[1] != 0)
		{
			throw new Exception("Prolog invalid");
		}
		num = 2;
		ParameterInfo[] parameters = GetParameters(constructorInfo);
		result.ctor = constructorInfo;
		result.ctorArgs = new object[parameters.Length];
		for (int i = 0; i < parameters.Length; i++)
		{
			result.ctorArgs[i] = decode_cattr_value(parameters[i].ParameterType, array, num, out num);
		}
		int num2 = array[num] + array[num + 1] * 256;
		num += 2;
		result.namedParamNames = new string[num2];
		result.namedParamValues = new object[num2];
		for (int j = 0; j < num2; j++)
		{
			int num3 = array[num++];
			byte num4 = array[num++];
			string text = null;
			if (num4 == 85)
			{
				int num5 = decode_len(array, num, out num);
				text = string_from_bytes(array, num, num5);
				num += num5;
			}
			int num6 = decode_len(array, num, out num);
			string text2 = string_from_bytes(array, num, num6);
			result.namedParamNames[j] = text2;
			num += num6;
			if (num3 == 83)
			{
				FieldInfo field = constructorInfo.DeclaringType.GetField(text2, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
				if (field == null)
				{
					throw new Exception(string.Concat("Custom attribute type '", constructorInfo.DeclaringType, "' doesn't contain a field named '", text2, "'"));
				}
				object obj = decode_cattr_value(field.FieldType, array, num, out num);
				if (text != null)
				{
					obj = Enum.ToObject(Type.GetType(text), obj);
				}
				result.namedParamValues[j] = obj;
				continue;
			}
			throw new Exception("Unknown named type: " + num3);
		}
		return result;
	}

	void _CustomAttributeBuilder.GetIDsOfNames([In] ref Guid riid, IntPtr rgszNames, uint cNames, uint lcid, IntPtr rgDispId)
	{
		throw new NotImplementedException();
	}

	void _CustomAttributeBuilder.GetTypeInfo(uint iTInfo, uint lcid, IntPtr ppTInfo)
	{
		throw new NotImplementedException();
	}

	void _CustomAttributeBuilder.GetTypeInfoCount(out uint pcTInfo)
	{
		throw new NotImplementedException();
	}

	void _CustomAttributeBuilder.Invoke(uint dispIdMember, [In] ref Guid riid, uint lcid, short wFlags, IntPtr pDispParams, IntPtr pVarResult, IntPtr pExcepInfo, IntPtr puArgErr)
	{
		throw new NotImplementedException();
	}

	private static ParameterInfo[] GetParameters(ConstructorInfo ctor)
	{
		ConstructorBuilder constructorBuilder = ctor as ConstructorBuilder;
		if (constructorBuilder != null)
		{
			return constructorBuilder.GetParametersInternal();
		}
		return ctor.GetParametersInternal();
	}
}
