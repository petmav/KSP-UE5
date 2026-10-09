using System.Collections.ObjectModel;
using System.Runtime.InteropServices;

namespace System.Reflection;

[Serializable]
[ComVisible(true)]
public struct CustomAttributeTypedArgument
{
	private Type argumentType;

	private object value;

	public Type ArgumentType => argumentType;

	public object Value => value;

	public CustomAttributeTypedArgument(Type argumentType, object value)
	{
		if (argumentType == null)
		{
			throw new ArgumentNullException("argumentType");
		}
		this.argumentType = argumentType;
		this.value = value;
		if (value is Array)
		{
			Array array = (Array)value;
			Type elementType = array.GetType().GetElementType();
			CustomAttributeTypedArgument[] array2 = new CustomAttributeTypedArgument[array.GetLength(0)];
			for (int i = 0; i < array2.Length; i++)
			{
				array2[i] = new CustomAttributeTypedArgument(elementType, array.GetValue(i));
			}
			this.value = new ReadOnlyCollection<CustomAttributeTypedArgument>(array2);
		}
	}

	public CustomAttributeTypedArgument(object value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		argumentType = value.GetType();
		this.value = value;
	}

	public override string ToString()
	{
		string text = ((value != null) ? value.ToString() : string.Empty);
		if (argumentType == typeof(string))
		{
			return "\"" + text + "\"";
		}
		if (argumentType == typeof(Type))
		{
			return "typeof (" + text + ")";
		}
		if (argumentType.IsEnum)
		{
			return "(" + argumentType.Name + ")" + text;
		}
		return text;
	}

	public override bool Equals(object obj)
	{
		if (!(obj is CustomAttributeTypedArgument customAttributeTypedArgument))
		{
			return false;
		}
		if (!(customAttributeTypedArgument.argumentType == argumentType) || value == null)
		{
			return customAttributeTypedArgument.value == null;
		}
		return value.Equals(customAttributeTypedArgument.value);
	}

	public override int GetHashCode()
	{
		return (argumentType.GetHashCode() << 16) + ((value != null) ? value.GetHashCode() : 0);
	}

	public static bool operator ==(CustomAttributeTypedArgument left, CustomAttributeTypedArgument right)
	{
		return left.Equals(right);
	}

	public static bool operator !=(CustomAttributeTypedArgument left, CustomAttributeTypedArgument right)
	{
		return !left.Equals(right);
	}
}
