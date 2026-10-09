using System.Globalization;
using System.Reflection;

namespace System.Xml.Xsl.Runtime;

internal class XmlExtensionFunction
{
	private string namespaceUri;

	private string name;

	private int numArgs;

	private Type objectType;

	private BindingFlags flags;

	private int hashCode;

	private MethodInfo meth;

	private Type[] argClrTypes;

	private Type retClrType;

	private XmlQueryType[] argXmlTypes;

	private XmlQueryType retXmlType;

	public MethodInfo Method => meth;

	public Type ClrReturnType => retClrType;

	public XmlQueryType XmlReturnType => retXmlType;

	public XmlExtensionFunction()
	{
	}

	public XmlExtensionFunction(string name, string namespaceUri, MethodInfo meth)
	{
		this.name = name;
		this.namespaceUri = namespaceUri;
		Bind(meth);
	}

	public XmlExtensionFunction(string name, string namespaceUri, int numArgs, Type objectType, BindingFlags flags)
	{
		Init(name, namespaceUri, numArgs, objectType, flags);
	}

	public void Init(string name, string namespaceUri, int numArgs, Type objectType, BindingFlags flags)
	{
		this.name = name;
		this.namespaceUri = namespaceUri;
		this.numArgs = numArgs;
		this.objectType = objectType;
		this.flags = flags;
		meth = null;
		argClrTypes = null;
		retClrType = null;
		argXmlTypes = null;
		retXmlType = null;
		hashCode = namespaceUri.GetHashCode() ^ name.GetHashCode() ^ ((int)flags << 16) ^ numArgs;
	}

	public Type GetClrArgumentType(int index)
	{
		return argClrTypes[index];
	}

	public XmlQueryType GetXmlArgumentType(int index)
	{
		return argXmlTypes[index];
	}

	public bool CanBind()
	{
		MethodInfo[] methods = objectType.GetMethods(flags);
		StringComparison comparisonType = (((flags & BindingFlags.IgnoreCase) != 0) ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal);
		MethodInfo[] array = methods;
		foreach (MethodInfo methodInfo in array)
		{
			if (methodInfo.Name.Equals(name, comparisonType) && (numArgs == -1 || methodInfo.GetParameters().Length == numArgs) && !methodInfo.IsGenericMethodDefinition)
			{
				return true;
			}
		}
		return false;
	}

	public void Bind()
	{
		MethodInfo[] methods = objectType.GetMethods(flags);
		MethodInfo methodInfo = null;
		StringComparison comparisonType = (((flags & BindingFlags.IgnoreCase) != 0) ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal);
		MethodInfo[] array = methods;
		foreach (MethodInfo methodInfo2 in array)
		{
			if (methodInfo2.Name.Equals(name, comparisonType) && (numArgs == -1 || methodInfo2.GetParameters().Length == numArgs))
			{
				if (methodInfo != null)
				{
					throw new XslTransformException("Ambiguous method call. Extension object '{0}' contains multiple '{1}' methods that have {2} parameter(s).", namespaceUri, name, numArgs.ToString(CultureInfo.InvariantCulture));
				}
				methodInfo = methodInfo2;
			}
		}
		if (methodInfo == null)
		{
			array = objectType.GetMethods(flags | BindingFlags.NonPublic);
			foreach (MethodInfo methodInfo3 in array)
			{
				if (methodInfo3.Name.Equals(name, comparisonType) && methodInfo3.GetParameters().Length == numArgs)
				{
					throw new XslTransformException("Method '{1}' of extension object '{0}' cannot be called because it is not public.", namespaceUri, name);
				}
			}
			throw new XslTransformException("Extension object '{0}' does not contain a matching '{1}' method that has {2} parameter(s).", namespaceUri, name, numArgs.ToString(CultureInfo.InvariantCulture));
		}
		if (methodInfo.IsGenericMethodDefinition)
		{
			throw new XslTransformException("Method '{1}' of extension object '{0}' cannot be called because it is generic.", namespaceUri, name);
		}
		Bind(methodInfo);
	}

	private void Bind(MethodInfo meth)
	{
		ParameterInfo[] parameters = meth.GetParameters();
		this.meth = meth;
		argClrTypes = new Type[parameters.Length];
		for (int i = 0; i < parameters.Length; i++)
		{
			argClrTypes[i] = GetClrType(parameters[i].ParameterType);
		}
		retClrType = GetClrType(this.meth.ReturnType);
		argXmlTypes = new XmlQueryType[parameters.Length];
		for (int i = 0; i < parameters.Length; i++)
		{
			argXmlTypes[i] = InferXmlType(argClrTypes[i]);
			if (namespaceUri.Length == 0)
			{
				if ((object)argXmlTypes[i] == XmlQueryTypeFactory.NodeNotRtf)
				{
					argXmlTypes[i] = XmlQueryTypeFactory.Node;
				}
				else if ((object)argXmlTypes[i] == XmlQueryTypeFactory.NodeSDod)
				{
					argXmlTypes[i] = XmlQueryTypeFactory.NodeS;
				}
			}
			else if ((object)argXmlTypes[i] == XmlQueryTypeFactory.NodeSDod)
			{
				argXmlTypes[i] = XmlQueryTypeFactory.NodeNotRtfS;
			}
		}
		retXmlType = InferXmlType(retClrType);
	}

	public object Invoke(object extObj, object[] args)
	{
		try
		{
			return meth.Invoke(extObj, flags, null, args, CultureInfo.InvariantCulture);
		}
		catch (TargetInvocationException ex)
		{
			throw new XslTransformException(ex.InnerException, "An error occurred during a call to extension function '{0}'. See InnerException for a complete description of the error.", name);
		}
		catch (Exception ex2)
		{
			if (!XmlException.IsCatchableException(ex2))
			{
				throw;
			}
			throw new XslTransformException(ex2, "An error occurred during a call to extension function '{0}'. See InnerException for a complete description of the error.", name);
		}
	}

	public override bool Equals(object other)
	{
		XmlExtensionFunction xmlExtensionFunction = other as XmlExtensionFunction;
		if (hashCode == xmlExtensionFunction.hashCode && name == xmlExtensionFunction.name && namespaceUri == xmlExtensionFunction.namespaceUri && numArgs == xmlExtensionFunction.numArgs && objectType == xmlExtensionFunction.objectType)
		{
			return flags == xmlExtensionFunction.flags;
		}
		return false;
	}

	public override int GetHashCode()
	{
		return hashCode;
	}

	private Type GetClrType(Type clrType)
	{
		if (clrType.IsEnum)
		{
			return Enum.GetUnderlyingType(clrType);
		}
		if (clrType.IsByRef)
		{
			throw new XslTransformException("Method '{1}' of extension object '{0}' cannot be called because it has one or more ByRef parameters.", namespaceUri, name);
		}
		return clrType;
	}

	private XmlQueryType InferXmlType(Type clrType)
	{
		return XsltConvert.InferXsltType(clrType);
	}
}
