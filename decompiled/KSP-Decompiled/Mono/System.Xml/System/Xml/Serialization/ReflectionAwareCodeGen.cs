using System.Collections;
using System.Globalization;
using System.Reflection;
using System.Text;

namespace System.Xml.Serialization;

internal class ReflectionAwareCodeGen
{
	private const string hexDigits = "0123456789ABCDEF";

	private const string arrayMemberKey = "0";

	private Hashtable reflectionVariables;

	private int nextReflectionVariableNumber;

	private IndentedWriter writer;

	private static string helperClassesForUseReflection = "\n    sealed class XSFieldInfo {{\n       {3} fieldInfo;\n        public XSFieldInfo({2} t, {1} memberName){{\n            fieldInfo = t.GetField(memberName);\n        }}\n        public {0} this[{0} o] {{\n            get {{\n                return fieldInfo.GetValue(o);\n            }}\n            set {{\n                fieldInfo.SetValue(o, value);\n            }}\n        }}\n\n    }}\n    sealed class XSPropInfo {{\n        {4} propInfo;\n        public XSPropInfo({2} t, {1} memberName){{\n            propInfo = t.GetProperty(memberName);\n        }}\n        public {0} this[{0} o] {{\n            get {{\n                return propInfo.GetValue(o, null);\n            }}\n            set {{\n                propInfo.SetValue(o, value, null);\n            }}\n        }}\n    }}\n    sealed class XSArrayInfo {{\n        {4} propInfo;\n        public XSArrayInfo({4} propInfo){{\n            this.propInfo = propInfo;\n        }}\n        public {0} this[{0} a, int i] {{\n            get {{\n                return propInfo.GetValue(a, new {0}[]{{i}});\n            }}\n            set {{\n                propInfo.SetValue(a, value, new {0}[]{{i}});\n            }}\n        }}\n    }}\n";

	internal ReflectionAwareCodeGen(IndentedWriter writer)
	{
		this.writer = writer;
	}

	internal void WriteReflectionInit(TypeScope scope)
	{
		foreach (Type type in scope.Types)
		{
			TypeDesc typeDesc = scope.GetTypeDesc(type);
			if (typeDesc.UseReflection)
			{
				WriteTypeInfo(scope, typeDesc, type);
			}
		}
	}

	private string WriteTypeInfo(TypeScope scope, TypeDesc typeDesc, Type type)
	{
		InitTheFirstTime();
		string cSharpName = typeDesc.CSharpName;
		string text = (string)reflectionVariables[cSharpName];
		if (text != null)
		{
			return text;
		}
		if (type.IsArray)
		{
			text = GenerateVariableName("array", typeDesc.CSharpName);
			TypeDesc arrayElementTypeDesc = typeDesc.ArrayElementTypeDesc;
			if (arrayElementTypeDesc.UseReflection)
			{
				string text2 = WriteTypeInfo(scope, arrayElementTypeDesc, scope.GetTypeFromTypeDesc(arrayElementTypeDesc));
				writer.WriteLine("static " + typeof(Type).FullName + " " + text + " = " + text2 + ".MakeArrayType();");
			}
			else
			{
				string text3 = WriteAssemblyInfo(type);
				writer.Write("static " + typeof(Type).FullName + " " + text + " = " + text3 + ".GetType(");
				WriteQuotedCSharpString(type.FullName);
				writer.WriteLine(");");
			}
		}
		else
		{
			text = GenerateVariableName("type", typeDesc.CSharpName);
			Type underlyingType = Nullable.GetUnderlyingType(type);
			if (underlyingType != null)
			{
				string text4 = WriteTypeInfo(scope, scope.GetTypeDesc(underlyingType), underlyingType);
				writer.WriteLine("static " + typeof(Type).FullName + " " + text + " = typeof(System.Nullable<>).MakeGenericType(new " + typeof(Type).FullName + "[] {" + text4 + "});");
			}
			else
			{
				string text5 = WriteAssemblyInfo(type);
				writer.Write("static " + typeof(Type).FullName + " " + text + " = " + text5 + ".GetType(");
				WriteQuotedCSharpString(type.FullName);
				writer.WriteLine(");");
			}
		}
		reflectionVariables.Add(cSharpName, text);
		TypeMapping typeMappingFromTypeDesc = scope.GetTypeMappingFromTypeDesc(typeDesc);
		if (typeMappingFromTypeDesc != null)
		{
			WriteMappingInfo(typeMappingFromTypeDesc, text, type);
		}
		if (typeDesc.IsCollection || typeDesc.IsEnumerable)
		{
			TypeDesc arrayElementTypeDesc2 = typeDesc.ArrayElementTypeDesc;
			if (arrayElementTypeDesc2.UseReflection)
			{
				WriteTypeInfo(scope, arrayElementTypeDesc2, scope.GetTypeFromTypeDesc(arrayElementTypeDesc2));
			}
			WriteCollectionInfo(text, typeDesc, type);
		}
		return text;
	}

	private void InitTheFirstTime()
	{
		if (reflectionVariables == null)
		{
			reflectionVariables = new Hashtable();
			writer.Write(string.Format(CultureInfo.InvariantCulture, helperClassesForUseReflection, "object", "string", typeof(Type).FullName, typeof(FieldInfo).FullName, typeof(PropertyInfo).FullName, typeof(MemberInfo).FullName, typeof(MemberTypes).FullName));
			WriteDefaultIndexerInit(typeof(IList), typeof(Array).FullName, collectionUseReflection: false, elementUseReflection: false);
		}
	}

	private void WriteMappingInfo(TypeMapping mapping, string typeVariable, Type type)
	{
		string cSharpName = mapping.TypeDesc.CSharpName;
		if (mapping is StructMapping)
		{
			StructMapping structMapping = mapping as StructMapping;
			for (int i = 0; i < structMapping.Members.Length; i++)
			{
				MemberMapping memberMapping = structMapping.Members[i];
				WriteMemberInfo(type, cSharpName, typeVariable, memberMapping.Name);
				if (memberMapping.CheckShouldPersist)
				{
					string memberName = "ShouldSerialize" + memberMapping.Name;
					WriteMethodInfo(cSharpName, typeVariable, memberName, false);
				}
				if (memberMapping.CheckSpecified != 0)
				{
					string memberName2 = memberMapping.Name + "Specified";
					WriteMemberInfo(type, cSharpName, typeVariable, memberName2);
				}
				if (memberMapping.ChoiceIdentifier != null)
				{
					string memberName3 = memberMapping.ChoiceIdentifier.MemberName;
					WriteMemberInfo(type, cSharpName, typeVariable, memberName3);
				}
			}
		}
		else if (mapping is EnumMapping)
		{
			FieldInfo[] fields = type.GetFields();
			for (int j = 0; j < fields.Length; j++)
			{
				WriteMemberInfo(type, cSharpName, typeVariable, fields[j].Name);
			}
		}
	}

	private void WriteCollectionInfo(string typeVariable, TypeDesc typeDesc, Type type)
	{
		string cSharpName = CodeIdentifier.GetCSharpName(type);
		string cSharpName2 = typeDesc.ArrayElementTypeDesc.CSharpName;
		bool useReflection = typeDesc.ArrayElementTypeDesc.UseReflection;
		if (typeDesc.IsCollection)
		{
			WriteDefaultIndexerInit(type, cSharpName, typeDesc.UseReflection, useReflection);
		}
		else if (typeDesc.IsEnumerable)
		{
			if (typeDesc.IsGenericInterface)
			{
				WriteMethodInfo(cSharpName, typeVariable, "System.Collections.Generic.IEnumerable*", true);
			}
			else if (!typeDesc.IsPrivateImplementation)
			{
				WriteMethodInfo(cSharpName, typeVariable, "GetEnumerator", true);
			}
		}
		WriteMethodInfo(cSharpName, typeVariable, "Add", false, GetStringForTypeof(cSharpName2, useReflection));
	}

	private string WriteAssemblyInfo(Type type)
	{
		string fullName = type.Assembly.FullName;
		string text = (string)reflectionVariables[fullName];
		if (text == null)
		{
			int num = fullName.IndexOf(',');
			string fullName2 = ((num > -1) ? fullName.Substring(0, num) : fullName);
			text = GenerateVariableName("assembly", fullName2);
			writer.Write("static " + typeof(Assembly).FullName + " " + text + " = ResolveDynamicAssembly(");
			WriteQuotedCSharpString(DynamicAssemblies.GetName(type.Assembly));
			writer.WriteLine(");");
			reflectionVariables.Add(fullName, text);
		}
		return text;
	}

	private string WriteMemberInfo(Type type, string escapedName, string typeVariable, string memberName)
	{
		MemberInfo[] member = type.GetMember(memberName);
		for (int i = 0; i < member.Length; i++)
		{
			switch (member[i].MemberType)
			{
			case MemberTypes.Property:
			{
				string text2 = GenerateVariableName("prop", memberName);
				writer.Write("static XSPropInfo " + text2 + " = new XSPropInfo(" + typeVariable + ", ");
				WriteQuotedCSharpString(memberName);
				writer.WriteLine(");");
				reflectionVariables.Add(memberName + ":" + escapedName, text2);
				return text2;
			}
			case MemberTypes.Field:
			{
				string text = GenerateVariableName("field", memberName);
				writer.Write("static XSFieldInfo " + text + " = new XSFieldInfo(" + typeVariable + ", ");
				WriteQuotedCSharpString(memberName);
				writer.WriteLine(");");
				reflectionVariables.Add(memberName + ":" + escapedName, text);
				return text;
			}
			}
		}
		throw new InvalidOperationException(Res.GetString("{0} is an unsupported type. Please use [XmlIgnore] attribute to exclude members of this type from serialization graph.", member[0].ToString()));
	}

	private string WriteMethodInfo(string escapedName, string typeVariable, string memberName, bool isNonPublic, params string[] paramTypes)
	{
		string text = GenerateVariableName("method", memberName);
		writer.Write("static " + typeof(MethodInfo).FullName + " " + text + " = " + typeVariable + ".GetMethod(");
		WriteQuotedCSharpString(memberName);
		writer.Write(", ");
		string fullName = typeof(BindingFlags).FullName;
		writer.Write(fullName);
		writer.Write(".Public | ");
		writer.Write(fullName);
		writer.Write(".Instance | ");
		writer.Write(fullName);
		writer.Write(".Static");
		if (isNonPublic)
		{
			writer.Write(" | ");
			writer.Write(fullName);
			writer.Write(".NonPublic");
		}
		writer.Write(", null, ");
		writer.Write("new " + typeof(Type).FullName + "[] { ");
		for (int i = 0; i < paramTypes.Length; i++)
		{
			writer.Write(paramTypes[i]);
			if (i < paramTypes.Length - 1)
			{
				writer.Write(", ");
			}
		}
		writer.WriteLine("}, null);");
		reflectionVariables.Add(memberName + ":" + escapedName, text);
		return text;
	}

	private string WriteDefaultIndexerInit(Type type, string escapedName, bool collectionUseReflection, bool elementUseReflection)
	{
		string text = GenerateVariableName("item", escapedName);
		PropertyInfo defaultIndexer = TypeScope.GetDefaultIndexer(type, null);
		writer.Write("static XSArrayInfo ");
		writer.Write(text);
		writer.Write("= new XSArrayInfo(");
		writer.Write(GetStringForTypeof(CodeIdentifier.GetCSharpName(type), collectionUseReflection));
		writer.Write(".GetProperty(");
		WriteQuotedCSharpString(defaultIndexer.Name);
		writer.Write(",");
		writer.Write(GetStringForTypeof(CodeIdentifier.GetCSharpName(defaultIndexer.PropertyType), elementUseReflection));
		writer.Write(",new ");
		writer.Write(typeof(Type[]).FullName);
		writer.WriteLine("{typeof(int)}));");
		reflectionVariables.Add("0:" + escapedName, text);
		return text;
	}

	private string GenerateVariableName(string prefix, string fullName)
	{
		nextReflectionVariableNumber++;
		return prefix + nextReflectionVariableNumber + "_" + CodeIdentifier.MakeValidInternal(fullName.Replace('.', '_'));
	}

	internal string GetReflectionVariable(string typeFullName, string memberName)
	{
		string key = ((memberName != null) ? (memberName + ":" + typeFullName) : typeFullName);
		return (string)reflectionVariables[key];
	}

	internal string GetStringForMethodInvoke(string obj, string escapedTypeName, string methodName, bool useReflection, params string[] args)
	{
		StringBuilder stringBuilder = new StringBuilder();
		if (useReflection)
		{
			stringBuilder.Append(GetReflectionVariable(escapedTypeName, methodName));
			stringBuilder.Append(".Invoke(");
			stringBuilder.Append(obj);
			stringBuilder.Append(", new object[] {");
		}
		else
		{
			stringBuilder.Append(obj);
			stringBuilder.Append(".@");
			stringBuilder.Append(methodName);
			stringBuilder.Append("(");
		}
		for (int i = 0; i < args.Length; i++)
		{
			if (i != 0)
			{
				stringBuilder.Append(", ");
			}
			stringBuilder.Append(args[i]);
		}
		if (useReflection)
		{
			stringBuilder.Append("})");
		}
		else
		{
			stringBuilder.Append(")");
		}
		return stringBuilder.ToString();
	}

	internal string GetStringForEnumCompare(EnumMapping mapping, string memberName, bool useReflection)
	{
		if (!useReflection)
		{
			CodeIdentifier.CheckValidIdentifier(memberName);
			return mapping.TypeDesc.CSharpName + ".@" + memberName;
		}
		string stringForEnumMember = GetStringForEnumMember(mapping.TypeDesc.CSharpName, memberName, useReflection);
		return GetStringForEnumLongValue(stringForEnumMember, useReflection);
	}

	internal string GetStringForEnumLongValue(string variable, bool useReflection)
	{
		if (useReflection)
		{
			return typeof(Convert).FullName + ".ToInt64(" + variable + ")";
		}
		return "((" + typeof(long).FullName + ")" + variable + ")";
	}

	internal string GetStringForTypeof(string typeFullName, bool useReflection)
	{
		if (useReflection)
		{
			return GetReflectionVariable(typeFullName, null);
		}
		return "typeof(" + typeFullName + ")";
	}

	internal string GetStringForMember(string obj, string memberName, TypeDesc typeDesc)
	{
		if (!typeDesc.UseReflection)
		{
			return obj + ".@" + memberName;
		}
		while (typeDesc != null)
		{
			string cSharpName = typeDesc.CSharpName;
			string reflectionVariable = GetReflectionVariable(cSharpName, memberName);
			if (reflectionVariable != null)
			{
				return reflectionVariable + "[" + obj + "]";
			}
			typeDesc = typeDesc.BaseTypeDesc;
			if (typeDesc != null && !typeDesc.UseReflection)
			{
				return "((" + typeDesc.CSharpName + ")" + obj + ").@" + memberName;
			}
		}
		return "[" + obj + "]";
	}

	internal string GetStringForEnumMember(string typeFullName, string memberName, bool useReflection)
	{
		if (!useReflection)
		{
			return typeFullName + ".@" + memberName;
		}
		return GetReflectionVariable(typeFullName, memberName) + "[null]";
	}

	internal string GetStringForArrayMember(string arrayName, string subscript, TypeDesc arrayTypeDesc)
	{
		if (!arrayTypeDesc.UseReflection)
		{
			return arrayName + "[" + subscript + "]";
		}
		string typeFullName = (arrayTypeDesc.IsCollection ? arrayTypeDesc.CSharpName : typeof(Array).FullName);
		string reflectionVariable = GetReflectionVariable(typeFullName, "0");
		return reflectionVariable + "[" + arrayName + ", " + subscript + "]";
	}

	internal string GetStringForMethod(string obj, string typeFullName, string memberName, bool useReflection)
	{
		if (!useReflection)
		{
			return obj + "." + memberName + "(";
		}
		return GetReflectionVariable(typeFullName, memberName) + ".Invoke(" + obj + ", new object[]{";
	}

	internal string GetStringForCreateInstance(string escapedTypeName, bool useReflection, bool ctorInaccessible, bool cast)
	{
		return GetStringForCreateInstance(escapedTypeName, useReflection, ctorInaccessible, cast, string.Empty);
	}

	internal string GetStringForCreateInstance(string escapedTypeName, bool useReflection, bool ctorInaccessible, bool cast, string arg)
	{
		if (!useReflection && !ctorInaccessible)
		{
			return "new " + escapedTypeName + "(" + arg + ")";
		}
		return GetStringForCreateInstance(GetStringForTypeof(escapedTypeName, useReflection), (cast && !useReflection) ? escapedTypeName : null, ctorInaccessible, arg);
	}

	internal string GetStringForCreateInstance(string type, string cast, bool nonPublic, string arg)
	{
		StringBuilder stringBuilder = new StringBuilder();
		if (cast != null && cast.Length > 0)
		{
			stringBuilder.Append("(");
			stringBuilder.Append(cast);
			stringBuilder.Append(")");
		}
		stringBuilder.Append(typeof(Activator).FullName);
		stringBuilder.Append(".CreateInstance(");
		stringBuilder.Append(type);
		stringBuilder.Append(", ");
		string fullName = typeof(BindingFlags).FullName;
		stringBuilder.Append(fullName);
		stringBuilder.Append(".Instance | ");
		stringBuilder.Append(fullName);
		stringBuilder.Append(".Public | ");
		stringBuilder.Append(fullName);
		stringBuilder.Append(".CreateInstance");
		if (nonPublic)
		{
			stringBuilder.Append(" | ");
			stringBuilder.Append(fullName);
			stringBuilder.Append(".NonPublic");
		}
		if (arg == null || arg.Length == 0)
		{
			stringBuilder.Append(", null, new object[0], null)");
		}
		else
		{
			stringBuilder.Append(", null, new object[] { ");
			stringBuilder.Append(arg);
			stringBuilder.Append(" }, null)");
		}
		return stringBuilder.ToString();
	}

	internal void WriteLocalDecl(string typeFullName, string variableName, string initValue, bool useReflection)
	{
		if (useReflection)
		{
			typeFullName = "object";
		}
		writer.Write(typeFullName);
		writer.Write(" ");
		writer.Write(variableName);
		if (initValue != null)
		{
			writer.Write(" = ");
			if (!useReflection && initValue != "null")
			{
				writer.Write("(" + typeFullName + ")");
			}
			writer.Write(initValue);
		}
		writer.WriteLine(";");
	}

	internal void WriteCreateInstance(string escapedName, string source, bool useReflection, bool ctorInaccessible)
	{
		writer.Write(useReflection ? "object" : escapedName);
		writer.Write(" ");
		writer.Write(source);
		writer.Write(" = ");
		writer.Write(GetStringForCreateInstance(escapedName, useReflection, ctorInaccessible, !useReflection && ctorInaccessible));
		writer.WriteLine(";");
	}

	internal void WriteInstanceOf(string source, string escapedTypeName, bool useReflection)
	{
		if (!useReflection)
		{
			writer.Write(source);
			writer.Write(" is ");
			writer.Write(escapedTypeName);
		}
		else
		{
			writer.Write(GetReflectionVariable(escapedTypeName, null));
			writer.Write(".IsAssignableFrom(");
			writer.Write(source);
			writer.Write(".GetType())");
		}
	}

	internal void WriteArrayLocalDecl(string typeName, string variableName, string initValue, TypeDesc arrayTypeDesc)
	{
		if (arrayTypeDesc.UseReflection)
		{
			typeName = (arrayTypeDesc.IsEnumerable ? typeof(IEnumerable).FullName : ((!arrayTypeDesc.IsCollection) ? typeof(Array).FullName : typeof(ICollection).FullName));
		}
		writer.Write(typeName);
		writer.Write(" ");
		writer.Write(variableName);
		if (initValue != null)
		{
			writer.Write(" = ");
			if (initValue != "null")
			{
				writer.Write("(" + typeName + ")");
			}
			writer.Write(initValue);
		}
		writer.WriteLine(";");
	}

	internal void WriteEnumCase(string fullTypeName, ConstantMapping c, bool useReflection)
	{
		writer.Write("case ");
		if (useReflection)
		{
			writer.Write(c.Value.ToString(CultureInfo.InvariantCulture));
		}
		else
		{
			writer.Write(fullTypeName);
			writer.Write(".@");
			CodeIdentifier.CheckValidIdentifier(c.Name);
			writer.Write(c.Name);
		}
		writer.Write(": ");
	}

	internal void WriteTypeCompare(string variable, string escapedTypeName, bool useReflection)
	{
		writer.Write(variable);
		writer.Write(" == ");
		writer.Write(GetStringForTypeof(escapedTypeName, useReflection));
	}

	internal void WriteArrayTypeCompare(string variable, string escapedTypeName, string elementTypeName, bool useReflection)
	{
		if (!useReflection)
		{
			writer.Write(variable);
			writer.Write(" == typeof(");
			writer.Write(escapedTypeName);
			writer.Write(")");
		}
		else
		{
			writer.Write(variable);
			writer.Write(".IsArray ");
			writer.Write(" && ");
			WriteTypeCompare(variable + ".GetElementType()", elementTypeName, useReflection);
		}
	}

	internal static void WriteQuotedCSharpString(IndentedWriter writer, string value)
	{
		if (value == null)
		{
			writer.Write("null");
			return;
		}
		writer.Write("@\"");
		foreach (char c in value)
		{
			if (c < ' ')
			{
				switch (c)
				{
				case '\r':
					writer.Write("\\r");
					continue;
				case '\n':
					writer.Write("\\n");
					continue;
				case '\t':
					writer.Write("\\t");
					continue;
				}
				byte b = (byte)c;
				writer.Write("\\x");
				writer.Write("0123456789ABCDEF"[b >> 4]);
				writer.Write("0123456789ABCDEF"[b & 0xF]);
			}
			else if (c == '"')
			{
				writer.Write("\"\"");
			}
			else
			{
				writer.Write(c);
			}
		}
		writer.Write("\"");
	}

	internal void WriteQuotedCSharpString(string value)
	{
		WriteQuotedCSharpString(writer, value);
	}
}
