using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.Reflection;
using System.Reflection.Emit;
using System.Text;
using System.Xml.Schema;

namespace System.Xml.Serialization;

internal class XmlSerializationWriterILGen : XmlSerializationILGen
{
	internal XmlSerializationWriterILGen(TypeScope[] scopes, string access, string className)
		: base(scopes, access, className)
	{
	}

	internal void GenerateBegin()
	{
		typeBuilder = CodeGenerator.CreateTypeBuilder(base.ModuleBuilder, base.ClassName, base.TypeAttributes | TypeAttributes.BeforeFieldInit, typeof(XmlSerializationWriter), CodeGenerator.EmptyTypeArray);
		TypeScope[] array = base.Scopes;
		foreach (TypeScope typeScope in array)
		{
			foreach (TypeMapping typeMapping in typeScope.TypeMappings)
			{
				if (typeMapping is StructMapping || typeMapping is EnumMapping)
				{
					base.MethodNames.Add(typeMapping, NextMethodName(typeMapping.TypeDesc.Name));
				}
			}
			base.RaCodeGen.WriteReflectionInit(typeScope);
		}
	}

	internal override void GenerateMethod(TypeMapping mapping)
	{
		if (!base.GeneratedMethods.Contains(mapping))
		{
			base.GeneratedMethods[mapping] = mapping;
			if (mapping is StructMapping)
			{
				WriteStructMethod((StructMapping)mapping);
			}
			else if (mapping is EnumMapping)
			{
				WriteEnumMethod((EnumMapping)mapping);
			}
		}
	}

	internal Type GenerateEnd()
	{
		GenerateReferencedMethods();
		GenerateInitCallbacksMethod();
		typeBuilder.DefineDefaultConstructor(CodeGenerator.PublicMethodAttributes);
		return typeBuilder.CreateType();
	}

	internal string GenerateElement(XmlMapping xmlMapping)
	{
		if (!xmlMapping.IsWriteable)
		{
			return null;
		}
		if (!xmlMapping.GenerateSerializer)
		{
			throw new ArgumentException(Res.GetString("Internal error."), "xmlMapping");
		}
		if (xmlMapping is XmlTypeMapping)
		{
			return GenerateTypeElement((XmlTypeMapping)xmlMapping);
		}
		if (xmlMapping is XmlMembersMapping)
		{
			return GenerateMembersElement((XmlMembersMapping)xmlMapping);
		}
		throw new ArgumentException(Res.GetString("Internal error."), "xmlMapping");
	}

	private void GenerateInitCallbacksMethod()
	{
		ilg = new CodeGenerator(typeBuilder);
		ilg.BeginMethod(typeof(void), "InitCallbacks", CodeGenerator.EmptyTypeArray, CodeGenerator.EmptyStringArray, CodeGenerator.ProtectedOverrideMethodAttributes);
		ilg.EndMethod();
	}

	private void WriteQualifiedNameElement(string name, string ns, object defaultValue, SourceInfo source, bool nullable, TypeMapping mapping)
	{
		int num;
		if (defaultValue != null)
		{
			num = ((defaultValue != DBNull.Value) ? 1 : 0);
			if (num != 0)
			{
				throw CodeGenerator.NotSupported("XmlQualifiedName DefaultValue not supported.  Fail in WriteValue()");
			}
		}
		else
		{
			num = 0;
		}
		List<Type> list = new List<Type>();
		ilg.Ldarg(0);
		ilg.Ldstr(name);
		list.Add(typeof(string));
		if (ns != null)
		{
			ilg.Ldstr(ns);
			list.Add(typeof(string));
		}
		source.Load(mapping.TypeDesc.Type);
		list.Add(mapping.TypeDesc.Type);
		MethodInfo method = typeof(XmlSerializationWriter).GetMethod(nullable ? "WriteNullableQualifiedNameLiteral" : "WriteElementQualifiedName", CodeGenerator.InstanceBindingFlags, null, list.ToArray(), null);
		ilg.Call(method);
		if (num != 0)
		{
			throw CodeGenerator.NotSupported("XmlQualifiedName DefaultValue not supported.  Fail in WriteValue()");
		}
	}

	private void WriteEnumValue(EnumMapping mapping, SourceInfo source, out Type returnType)
	{
		string methodName = ReferenceMapping(mapping);
		MethodBuilder methodInfo = EnsureMethodBuilder(typeBuilder, methodName, CodeGenerator.PrivateMethodAttributes, typeof(string), new Type[1] { mapping.TypeDesc.Type });
		ilg.Ldarg(0);
		source.Load(mapping.TypeDesc.Type);
		ilg.Call(methodInfo);
		returnType = typeof(string);
	}

	private void WritePrimitiveValue(TypeDesc typeDesc, SourceInfo source, out Type returnType)
	{
		if (typeDesc == base.StringTypeDesc || typeDesc.FormatterName == "String")
		{
			source.Load(typeDesc.Type);
			returnType = typeDesc.Type;
			return;
		}
		if (!typeDesc.HasCustomFormatter)
		{
			Type type = typeDesc.Type;
			MethodInfo method = typeof(XmlConvert).GetMethod("ToString", CodeGenerator.StaticBindingFlags, null, new Type[1] { type }, null);
			source.Load(typeDesc.Type);
			ilg.Call(method);
			returnType = method.ReturnType;
			return;
		}
		BindingFlags bindingAttr = CodeGenerator.StaticBindingFlags;
		if (typeDesc.FormatterName == "XmlQualifiedName")
		{
			bindingAttr = CodeGenerator.InstanceBindingFlags;
			ilg.Ldarg(0);
		}
		MethodInfo method2 = typeof(XmlSerializationWriter).GetMethod("From" + typeDesc.FormatterName, bindingAttr, null, new Type[1] { typeDesc.Type }, null);
		source.Load(typeDesc.Type);
		ilg.Call(method2);
		returnType = method2.ReturnType;
	}

	private void WritePrimitive(string method, string name, string ns, object defaultValue, SourceInfo source, TypeMapping mapping, bool writeXsiType, bool isElement, bool isNullable)
	{
		TypeDesc typeDesc = mapping.TypeDesc;
		bool flag = defaultValue != null && defaultValue != DBNull.Value && mapping.TypeDesc.HasDefaultSupport;
		if (flag)
		{
			if (mapping is EnumMapping)
			{
				source.Load(mapping.TypeDesc.Type);
				string text = null;
				if (((EnumMapping)mapping).IsFlags)
				{
					string[] array = ((string)defaultValue).Split(null);
					for (int i = 0; i < array.Length; i++)
					{
						if (array[i] != null && array[i].Length != 0)
						{
							if (i > 0)
							{
								text += ", ";
							}
							text += array[i];
						}
					}
				}
				else
				{
					text = (string)defaultValue;
				}
				ilg.Ldc(Enum.Parse(mapping.TypeDesc.Type, text, ignoreCase: false));
				ilg.If(Cmp.NotEqualTo);
			}
			else
			{
				WriteCheckDefault(source, defaultValue, isNullable);
			}
		}
		List<Type> list = new List<Type>();
		ilg.Ldarg(0);
		list.Add(typeof(string));
		ilg.Ldstr(name);
		if (ns != null)
		{
			list.Add(typeof(string));
			ilg.Ldstr(ns);
		}
		Type returnType;
		if (mapping is EnumMapping)
		{
			WriteEnumValue((EnumMapping)mapping, source, out returnType);
			list.Add(returnType);
		}
		else
		{
			WritePrimitiveValue(typeDesc, source, out returnType);
			list.Add(returnType);
		}
		if (writeXsiType)
		{
			list.Add(typeof(XmlQualifiedName));
			ConstructorInfo constructor = typeof(XmlQualifiedName).GetConstructor(CodeGenerator.InstanceBindingFlags, null, new Type[2]
			{
				typeof(string),
				typeof(string)
			}, null);
			ilg.Ldstr(mapping.TypeName);
			ilg.Ldstr(mapping.Namespace);
			ilg.New(constructor);
		}
		MethodInfo method2 = typeof(XmlSerializationWriter).GetMethod(method, CodeGenerator.InstanceBindingFlags, null, list.ToArray(), null);
		ilg.Call(method2);
		if (flag)
		{
			ilg.EndIf();
		}
	}

	private void WriteTag(string methodName, string name, string ns)
	{
		MethodInfo method = typeof(XmlSerializationWriter).GetMethod(methodName, CodeGenerator.InstanceBindingFlags, null, new Type[2]
		{
			typeof(string),
			typeof(string)
		}, null);
		ilg.Ldarg(0);
		ilg.Ldstr(name);
		ilg.Ldstr(ns);
		ilg.Call(method);
	}

	private void WriteTag(string methodName, string name, string ns, bool writePrefixed)
	{
		MethodInfo method = typeof(XmlSerializationWriter).GetMethod(methodName, CodeGenerator.InstanceBindingFlags, null, new Type[4]
		{
			typeof(string),
			typeof(string),
			typeof(object),
			typeof(bool)
		}, null);
		ilg.Ldarg(0);
		ilg.Ldstr(name);
		ilg.Ldstr(ns);
		ilg.Load(null);
		ilg.Ldc(writePrefixed);
		ilg.Call(method);
	}

	private void WriteStartElement(string name, string ns, bool writePrefixed)
	{
		WriteTag("WriteStartElement", name, ns, writePrefixed);
	}

	private void WriteEndElement()
	{
		MethodInfo method = typeof(XmlSerializationWriter).GetMethod("WriteEndElement", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg.Ldarg(0);
		ilg.Call(method);
	}

	private void WriteEndElement(string source)
	{
		MethodInfo method = typeof(XmlSerializationWriter).GetMethod("WriteEndElement", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(object) }, null);
		object variable = ilg.GetVariable(source);
		ilg.Ldarg(0);
		ilg.Load(variable);
		ilg.ConvertValue(ilg.GetVariableType(variable), typeof(object));
		ilg.Call(method);
	}

	private void WriteLiteralNullTag(string name, string ns)
	{
		WriteTag("WriteNullTagLiteral", name, ns);
	}

	private void WriteEmptyTag(string name, string ns)
	{
		WriteTag("WriteEmptyTag", name, ns);
	}

	private string GenerateMembersElement(XmlMembersMapping xmlMembersMapping)
	{
		ElementAccessor accessor = xmlMembersMapping.Accessor;
		MembersMapping membersMapping = (MembersMapping)accessor.Mapping;
		bool hasWrapperElement = membersMapping.HasWrapperElement;
		bool writeAccessors = membersMapping.WriteAccessors;
		string text = NextMethodName(accessor.Name);
		ilg = new CodeGenerator(typeBuilder);
		ilg.BeginMethod(typeof(void), text, new Type[1] { typeof(object[]) }, new string[1] { "p" }, CodeGenerator.PublicMethodAttributes);
		MethodInfo method = typeof(XmlSerializationWriter).GetMethod("WriteStartDocument", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg.Ldarg(0);
		ilg.Call(method);
		MethodInfo method2 = typeof(XmlSerializationWriter).GetMethod("TopLevelElement", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg.Ldarg(0);
		ilg.Call(method2);
		LocalBuilder localBuilder = ilg.DeclareLocal(typeof(int), "pLength");
		ilg.Ldarg("p");
		ilg.Ldlen();
		ilg.Stloc(localBuilder);
		if (hasWrapperElement)
		{
			WriteStartElement(accessor.Name, (accessor.Form == XmlSchemaForm.Qualified) ? accessor.Namespace : "", writePrefixed: false);
			int num = FindXmlnsIndex(membersMapping.Members);
			if (num >= 0)
			{
				_ = membersMapping.Members[num];
				string source = "((" + typeof(XmlSerializerNamespaces).FullName + ")p[" + num.ToString(CultureInfo.InvariantCulture) + "])";
				ilg.Ldloc(localBuilder);
				ilg.Ldc(num);
				ilg.If(Cmp.GreaterThan);
				WriteNamespaces(source);
				ilg.EndIf();
			}
			for (int i = 0; i < membersMapping.Members.Length; i++)
			{
				MemberMapping memberMapping = membersMapping.Members[i];
				if (memberMapping.Attribute == null || memberMapping.Ignore)
				{
					continue;
				}
				SourceInfo source2 = new SourceInfo("p[" + i.ToString(CultureInfo.InvariantCulture) + "]", null, null, localBuilder.LocalType.GetElementType(), ilg);
				SourceInfo sourceInfo = null;
				int intVar = 0;
				if (memberMapping.CheckSpecified != 0)
				{
					string text2 = memberMapping.Name + "Specified";
					for (int j = 0; j < membersMapping.Members.Length; j++)
					{
						if (membersMapping.Members[j].Name == text2)
						{
							sourceInfo = new SourceInfo("((bool)p[" + j.ToString(CultureInfo.InvariantCulture) + "])", null, null, typeof(bool), ilg);
							intVar = j;
							break;
						}
					}
				}
				ilg.Ldloc(localBuilder);
				ilg.Ldc(i);
				ilg.If(Cmp.GreaterThan);
				if (sourceInfo != null)
				{
					Label label = ilg.DefineLabel();
					Label label2 = ilg.DefineLabel();
					ilg.Ldloc(localBuilder);
					ilg.Ldc(intVar);
					ilg.Ble(label);
					sourceInfo.Load(typeof(bool));
					ilg.Br_S(label2);
					ilg.MarkLabel(label);
					ilg.Ldc(boolVar: true);
					ilg.MarkLabel(label2);
					ilg.If();
				}
				WriteMember(source2, memberMapping.Attribute, memberMapping.TypeDesc, "p");
				if (sourceInfo != null)
				{
					ilg.EndIf();
				}
				ilg.EndIf();
			}
		}
		for (int k = 0; k < membersMapping.Members.Length; k++)
		{
			MemberMapping memberMapping2 = membersMapping.Members[k];
			if (memberMapping2.Xmlns != null || memberMapping2.Ignore)
			{
				continue;
			}
			SourceInfo sourceInfo2 = null;
			int intVar2 = 0;
			if (memberMapping2.CheckSpecified != 0)
			{
				string text3 = memberMapping2.Name + "Specified";
				for (int l = 0; l < membersMapping.Members.Length; l++)
				{
					if (membersMapping.Members[l].Name == text3)
					{
						sourceInfo2 = new SourceInfo("((bool)p[" + l.ToString(CultureInfo.InvariantCulture) + "])", null, null, typeof(bool), ilg);
						intVar2 = l;
						break;
					}
				}
			}
			ilg.Ldloc(localBuilder);
			ilg.Ldc(k);
			ilg.If(Cmp.GreaterThan);
			if (sourceInfo2 != null)
			{
				Label label3 = ilg.DefineLabel();
				Label label4 = ilg.DefineLabel();
				ilg.Ldloc(localBuilder);
				ilg.Ldc(intVar2);
				ilg.Ble(label3);
				sourceInfo2.Load(typeof(bool));
				ilg.Br_S(label4);
				ilg.MarkLabel(label3);
				ilg.Ldc(boolVar: true);
				ilg.MarkLabel(label4);
				ilg.If();
			}
			string text4 = "p[" + k.ToString(CultureInfo.InvariantCulture) + "]";
			string choiceSource = null;
			if (memberMapping2.ChoiceIdentifier != null)
			{
				for (int m = 0; m < membersMapping.Members.Length; m++)
				{
					if (membersMapping.Members[m].Name == memberMapping2.ChoiceIdentifier.MemberName)
					{
						choiceSource = "((" + membersMapping.Members[m].TypeDesc.CSharpName + ")p[" + m.ToString(CultureInfo.InvariantCulture) + "])";
						break;
					}
				}
			}
			WriteMember(new SourceInfo(text4, text4, null, null, ilg), choiceSource, memberMapping2.ElementsSortedByDerivation, memberMapping2.Text, memberMapping2.ChoiceIdentifier, memberMapping2.TypeDesc, writeAccessors || hasWrapperElement);
			if (sourceInfo2 != null)
			{
				ilg.EndIf();
			}
			ilg.EndIf();
		}
		if (hasWrapperElement)
		{
			WriteEndElement();
		}
		ilg.EndMethod();
		return text;
	}

	private string GenerateTypeElement(XmlTypeMapping xmlTypeMapping)
	{
		ElementAccessor accessor = xmlTypeMapping.Accessor;
		TypeMapping mapping = accessor.Mapping;
		string text = NextMethodName(accessor.Name);
		ilg = new CodeGenerator(typeBuilder);
		ilg.BeginMethod(typeof(void), text, new Type[1] { typeof(object) }, new string[1] { "o" }, CodeGenerator.PublicMethodAttributes);
		MethodInfo method = typeof(XmlSerializationWriter).GetMethod("WriteStartDocument", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg.Ldarg(0);
		ilg.Call(method);
		ilg.If(ilg.GetArg("o"), Cmp.EqualTo, null);
		if (accessor.IsNullable)
		{
			WriteLiteralNullTag(accessor.Name, (accessor.Form == XmlSchemaForm.Qualified) ? accessor.Namespace : "");
		}
		else
		{
			WriteEmptyTag(accessor.Name, (accessor.Form == XmlSchemaForm.Qualified) ? accessor.Namespace : "");
		}
		ilg.GotoMethodEnd();
		ilg.EndIf();
		if (!mapping.TypeDesc.IsValueType && !mapping.TypeDesc.Type.IsPrimitive)
		{
			MethodInfo method2 = typeof(XmlSerializationWriter).GetMethod("TopLevelElement", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldarg(0);
			ilg.Call(method2);
		}
		WriteMember(new SourceInfo("o", "o", null, typeof(object), ilg), null, new ElementAccessor[1] { accessor }, null, null, mapping.TypeDesc, writeAccessors: true);
		ilg.EndMethod();
		return text;
	}

	private string NextMethodName(string name)
	{
		return "Write" + (++base.NextMethodNumber).ToString(null, NumberFormatInfo.InvariantInfo) + "_" + CodeIdentifier.MakeValidInternal(name);
	}

	private void WriteEnumMethod(EnumMapping mapping)
	{
		string methodName = (string)base.MethodNames[mapping];
		List<Type> list = new List<Type>();
		List<string> list2 = new List<string>();
		list.Add(mapping.TypeDesc.Type);
		list2.Add("v");
		ilg = new CodeGenerator(typeBuilder);
		ilg.BeginMethod(typeof(string), GetMethodBuilder(methodName), list.ToArray(), list2.ToArray(), CodeGenerator.PrivateMethodAttributes);
		LocalBuilder localBuilder = ilg.DeclareLocal(typeof(string), "s");
		ilg.Load(null);
		ilg.Stloc(localBuilder);
		ConstantMapping[] constants = mapping.Constants;
		if (constants.Length != 0)
		{
			Hashtable hashtable = new Hashtable();
			List<Label> list3 = new List<Label>();
			List<string> list4 = new List<string>();
			Label label = ilg.DefineLabel();
			Label label2 = ilg.DefineLabel();
			LocalBuilder localBuilder2 = ilg.DeclareLocal(mapping.TypeDesc.Type, "localTmp");
			ilg.Ldarg("v");
			ilg.Stloc(localBuilder2);
			foreach (ConstantMapping constantMapping in constants)
			{
				if (hashtable[constantMapping.Value] == null)
				{
					Label label3 = ilg.DefineLabel();
					ilg.Ldloc(localBuilder2);
					ilg.Ldc(Enum.ToObject(mapping.TypeDesc.Type, constantMapping.Value));
					ilg.Beq(label3);
					list3.Add(label3);
					list4.Add(constantMapping.XmlName);
					hashtable.Add(constantMapping.Value, constantMapping.Value);
				}
			}
			if (mapping.IsFlags)
			{
				ilg.Br(label);
				for (int j = 0; j < list3.Count; j++)
				{
					ilg.MarkLabel(list3[j]);
					ilg.Ldc(list4[j]);
					ilg.Stloc(localBuilder);
					ilg.Br(label2);
				}
				ilg.MarkLabel(label);
				base.RaCodeGen.ILGenForEnumLongValue(ilg, "v");
				LocalBuilder localBuilder3 = ilg.DeclareLocal(typeof(string[]), "strArray");
				ilg.NewArray(typeof(string), constants.Length);
				ilg.Stloc(localBuilder3);
				for (int k = 0; k < constants.Length; k++)
				{
					ConstantMapping constantMapping2 = constants[k];
					ilg.Ldloc(localBuilder3);
					ilg.Ldc(k);
					ilg.Ldstr(constantMapping2.XmlName);
					ilg.Stelem(typeof(string));
				}
				ilg.Ldloc(localBuilder3);
				LocalBuilder localBuilder4 = ilg.DeclareLocal(typeof(long[]), "longArray");
				ilg.NewArray(typeof(long), constants.Length);
				ilg.Stloc(localBuilder4);
				for (int l = 0; l < constants.Length; l++)
				{
					ConstantMapping constantMapping3 = constants[l];
					ilg.Ldloc(localBuilder4);
					ilg.Ldc(l);
					ilg.Ldc(constantMapping3.Value);
					ilg.Stelem(typeof(long));
				}
				ilg.Ldloc(localBuilder4);
				ilg.Ldstr(mapping.TypeDesc.FullName);
				MethodInfo method = typeof(XmlSerializationWriter).GetMethod("FromEnum", CodeGenerator.StaticBindingFlags, null, new Type[4]
				{
					typeof(long),
					typeof(string[]),
					typeof(long[]),
					typeof(string)
				}, null);
				ilg.Call(method);
				ilg.Stloc(localBuilder);
				ilg.Br(label2);
			}
			else
			{
				ilg.Br(label);
				for (int m = 0; m < list3.Count; m++)
				{
					ilg.MarkLabel(list3[m]);
					ilg.Ldc(list4[m]);
					ilg.Stloc(localBuilder);
					ilg.Br(label2);
				}
				MethodInfo method2 = typeof(CultureInfo).GetMethod("get_InvariantCulture", CodeGenerator.StaticBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				MethodInfo method3 = typeof(long).GetMethod("ToString", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(IFormatProvider) }, null);
				MethodInfo method4 = typeof(XmlSerializationWriter).GetMethod("CreateInvalidEnumValueException", CodeGenerator.InstanceBindingFlags, null, new Type[2]
				{
					typeof(object),
					typeof(string)
				}, null);
				ilg.MarkLabel(label);
				ilg.Ldarg(0);
				ilg.Ldarg("v");
				ilg.ConvertValue(mapping.TypeDesc.Type, typeof(long));
				LocalBuilder localBuilder5 = ilg.DeclareLocal(typeof(long), "num");
				ilg.Stloc(localBuilder5);
				ilg.LdlocAddress(localBuilder5);
				ilg.Call(method2);
				ilg.Call(method3);
				ilg.Ldstr(mapping.TypeDesc.FullName);
				ilg.Call(method4);
				ilg.Throw();
			}
			ilg.MarkLabel(label2);
		}
		ilg.Ldloc(localBuilder);
		ilg.EndMethod();
	}

	private void WriteDerivedTypes(StructMapping mapping)
	{
		for (StructMapping structMapping = mapping.DerivedMappings; structMapping != null; structMapping = structMapping.NextDerivedMapping)
		{
			ilg.InitElseIf();
			WriteTypeCompare("t", structMapping.TypeDesc.Type);
			ilg.AndIf();
			string methodName = ReferenceMapping(structMapping);
			List<Type> list = new List<Type>();
			ilg.Ldarg(0);
			list.Add(typeof(string));
			ilg.Ldarg("n");
			list.Add(typeof(string));
			ilg.Ldarg("ns");
			object variable = ilg.GetVariable("o");
			Type variableType = ilg.GetVariableType(variable);
			ilg.Load(variable);
			ilg.ConvertValue(variableType, structMapping.TypeDesc.Type);
			list.Add(structMapping.TypeDesc.Type);
			if (structMapping.TypeDesc.IsNullable)
			{
				list.Add(typeof(bool));
				ilg.Ldarg("isNullable");
			}
			list.Add(typeof(bool));
			ilg.Ldc(boolVar: true);
			MethodInfo methodInfo = EnsureMethodBuilder(typeBuilder, methodName, CodeGenerator.PrivateMethodAttributes, typeof(void), list.ToArray());
			ilg.Call(methodInfo);
			ilg.GotoMethodEnd();
			WriteDerivedTypes(structMapping);
		}
	}

	private void WriteEnumAndArrayTypes()
	{
		TypeScope[] array = base.Scopes;
		for (int i = 0; i < array.Length; i++)
		{
			foreach (Mapping typeMapping in array[i].TypeMappings)
			{
				if (typeMapping is EnumMapping)
				{
					EnumMapping enumMapping = (EnumMapping)typeMapping;
					ilg.InitElseIf();
					WriteTypeCompare("t", enumMapping.TypeDesc.Type);
					ilg.AndIf();
					string methodName = ReferenceMapping(enumMapping);
					MethodInfo method = typeof(XmlSerializationWriter).GetMethod("get_Writer", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
					MethodInfo method2 = typeof(XmlWriter).GetMethod("WriteStartElement", CodeGenerator.InstanceBindingFlags, null, new Type[2]
					{
						typeof(string),
						typeof(string)
					}, null);
					ilg.Ldarg(0);
					ilg.Call(method);
					ilg.Ldarg("n");
					ilg.Ldarg("ns");
					ilg.Call(method2);
					MethodInfo method3 = typeof(XmlSerializationWriter).GetMethod("WriteXsiType", CodeGenerator.InstanceBindingFlags, null, new Type[2]
					{
						typeof(string),
						typeof(string)
					}, null);
					ilg.Ldarg(0);
					ilg.Ldstr(enumMapping.TypeName);
					ilg.Ldstr(enumMapping.Namespace);
					ilg.Call(method3);
					MethodBuilder methodInfo = EnsureMethodBuilder(typeBuilder, methodName, CodeGenerator.PrivateMethodAttributes, typeof(string), new Type[1] { enumMapping.TypeDesc.Type });
					MethodInfo method4 = typeof(XmlWriter).GetMethod("WriteString", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(string) }, null);
					ilg.Ldarg(0);
					ilg.Call(method);
					object variable = ilg.GetVariable("o");
					ilg.Ldarg(0);
					ilg.Load(variable);
					ilg.ConvertValue(ilg.GetVariableType(variable), enumMapping.TypeDesc.Type);
					ilg.Call(methodInfo);
					ilg.Call(method4);
					MethodInfo method5 = typeof(XmlWriter).GetMethod("WriteEndElement", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
					ilg.Ldarg(0);
					ilg.Call(method);
					ilg.Call(method5);
					ilg.GotoMethodEnd();
				}
				else if (typeMapping is ArrayMapping && typeMapping is ArrayMapping arrayMapping)
				{
					ilg.InitElseIf();
					if (arrayMapping.TypeDesc.IsArray)
					{
						WriteArrayTypeCompare("t", arrayMapping.TypeDesc.Type);
					}
					else
					{
						WriteTypeCompare("t", arrayMapping.TypeDesc.Type);
					}
					ilg.AndIf();
					ilg.EnterScope();
					MethodInfo method6 = typeof(XmlSerializationWriter).GetMethod("get_Writer", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
					MethodInfo method7 = typeof(XmlWriter).GetMethod("WriteStartElement", CodeGenerator.InstanceBindingFlags, null, new Type[2]
					{
						typeof(string),
						typeof(string)
					}, null);
					ilg.Ldarg(0);
					ilg.Call(method6);
					ilg.Ldarg("n");
					ilg.Ldarg("ns");
					ilg.Call(method7);
					MethodInfo method8 = typeof(XmlSerializationWriter).GetMethod("WriteXsiType", CodeGenerator.InstanceBindingFlags, null, new Type[2]
					{
						typeof(string),
						typeof(string)
					}, null);
					ilg.Ldarg(0);
					ilg.Ldstr(arrayMapping.TypeName);
					ilg.Ldstr(arrayMapping.Namespace);
					ilg.Call(method8);
					WriteMember(new SourceInfo("o", "o", null, null, ilg), null, arrayMapping.ElementsSortedByDerivation, null, null, arrayMapping.TypeDesc, writeAccessors: true);
					MethodInfo method9 = typeof(XmlWriter).GetMethod("WriteEndElement", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
					ilg.Ldarg(0);
					ilg.Call(method6);
					ilg.Call(method9);
					ilg.GotoMethodEnd();
					ilg.ExitScope();
				}
			}
		}
	}

	private void WriteStructMethod(StructMapping mapping)
	{
		string methodName = (string)base.MethodNames[mapping];
		ilg = new CodeGenerator(typeBuilder);
		List<Type> list = new List<Type>(5);
		List<string> list2 = new List<string>(5);
		list.Add(typeof(string));
		list2.Add("n");
		list.Add(typeof(string));
		list2.Add("ns");
		list.Add(mapping.TypeDesc.Type);
		list2.Add("o");
		if (mapping.TypeDesc.IsNullable)
		{
			list.Add(typeof(bool));
			list2.Add("isNullable");
		}
		list.Add(typeof(bool));
		list2.Add("needType");
		ilg.BeginMethod(typeof(void), GetMethodBuilder(methodName), list.ToArray(), list2.ToArray(), CodeGenerator.PrivateMethodAttributes);
		if (mapping.TypeDesc.IsNullable)
		{
			ilg.If(ilg.GetArg("o"), Cmp.EqualTo, null);
			ilg.If(ilg.GetArg("isNullable"), Cmp.EqualTo, true);
			MethodInfo method = typeof(XmlSerializationWriter).GetMethod("WriteNullTagLiteral", CodeGenerator.InstanceBindingFlags, null, new Type[2]
			{
				typeof(string),
				typeof(string)
			}, null);
			ilg.Ldarg(0);
			ilg.Ldarg("n");
			ilg.Ldarg("ns");
			ilg.Call(method);
			ilg.EndIf();
			ilg.GotoMethodEnd();
			ilg.EndIf();
		}
		ilg.If(ilg.GetArg("needType"), Cmp.NotEqualTo, true);
		LocalBuilder local = ilg.DeclareLocal(typeof(Type), "t");
		MethodInfo method2 = typeof(object).GetMethod("GetType", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ArgBuilder arg = ilg.GetArg("o");
		ilg.LdargAddress(arg);
		ilg.ConvertAddress(arg.ArgType, typeof(object));
		ilg.Call(method2);
		ilg.Stloc(local);
		WriteTypeCompare("t", mapping.TypeDesc.Type);
		ilg.If();
		WriteDerivedTypes(mapping);
		if (mapping.TypeDesc.IsRoot)
		{
			WriteEnumAndArrayTypes();
		}
		ilg.Else();
		if (mapping.TypeDesc.IsRoot)
		{
			MethodInfo method3 = typeof(XmlSerializationWriter).GetMethod("WriteTypedPrimitive", CodeGenerator.InstanceBindingFlags, null, new Type[4]
			{
				typeof(string),
				typeof(string),
				typeof(object),
				typeof(bool)
			}, null);
			ilg.Ldarg(0);
			ilg.Ldarg("n");
			ilg.Ldarg("ns");
			ilg.Ldarg("o");
			ilg.Ldc(boolVar: true);
			ilg.Call(method3);
			ilg.GotoMethodEnd();
		}
		else
		{
			MethodInfo method4 = typeof(XmlSerializationWriter).GetMethod("CreateUnknownTypeException", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(object) }, null);
			ilg.Ldarg(0);
			ilg.Ldarg(arg);
			ilg.ConvertValue(arg.ArgType, typeof(object));
			ilg.Call(method4);
			ilg.Throw();
		}
		ilg.EndIf();
		ilg.EndIf();
		if (!mapping.TypeDesc.IsAbstract)
		{
			if (mapping.TypeDesc.Type != null && typeof(XmlSchemaObject).IsAssignableFrom(mapping.TypeDesc.Type))
			{
				MethodInfo method5 = typeof(XmlSerializationWriter).GetMethod("set_EscapeName", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(bool) }, null);
				ilg.Ldarg(0);
				ilg.Ldc(boolVar: false);
				ilg.Call(method5);
			}
			string text = null;
			MemberMapping[] allMembers = TypeScope.GetAllMembers(mapping, memberInfos);
			int num = FindXmlnsIndex(allMembers);
			if (num >= 0)
			{
				MemberMapping memberMapping = allMembers[num];
				CodeIdentifier.CheckValidIdentifier(memberMapping.Name);
				text = base.RaCodeGen.GetStringForMember("o", memberMapping.Name, mapping.TypeDesc);
			}
			ilg.Ldarg(0);
			ilg.Ldarg("n");
			ilg.Ldarg("ns");
			ArgBuilder arg2 = ilg.GetArg("o");
			ilg.Ldarg(arg2);
			ilg.ConvertValue(arg2.ArgType, typeof(object));
			ilg.Ldc(boolVar: false);
			if (text == null)
			{
				ilg.Load(null);
			}
			else
			{
				ILGenLoad(text);
			}
			MethodInfo method6 = typeof(XmlSerializationWriter).GetMethod("WriteStartElement", CodeGenerator.InstanceBindingFlags, null, new Type[5]
			{
				typeof(string),
				typeof(string),
				typeof(object),
				typeof(bool),
				typeof(XmlSerializerNamespaces)
			}, null);
			ilg.Call(method6);
			if (!mapping.TypeDesc.IsRoot)
			{
				ilg.If(ilg.GetArg("needType"), Cmp.EqualTo, true);
				MethodInfo method7 = typeof(XmlSerializationWriter).GetMethod("WriteXsiType", CodeGenerator.InstanceBindingFlags, null, new Type[2]
				{
					typeof(string),
					typeof(string)
				}, null);
				ilg.Ldarg(0);
				ilg.Ldstr(mapping.TypeName);
				ilg.Ldstr(mapping.Namespace);
				ilg.Call(method7);
				ilg.EndIf();
			}
			foreach (MemberMapping memberMapping2 in allMembers)
			{
				if (memberMapping2.Attribute != null)
				{
					CodeIdentifier.CheckValidIdentifier(memberMapping2.Name);
					if (memberMapping2.CheckShouldPersist)
					{
						ilg.LdargAddress(arg);
						ilg.Call(memberMapping2.CheckShouldPersistMethodInfo);
						ilg.If();
					}
					if (memberMapping2.CheckSpecified != 0)
					{
						string stringForMember = base.RaCodeGen.GetStringForMember("o", memberMapping2.Name + "Specified", mapping.TypeDesc);
						ILGenLoad(stringForMember);
						ilg.If();
					}
					WriteMember(base.RaCodeGen.GetSourceForMember("o", memberMapping2, mapping.TypeDesc, ilg), memberMapping2.Attribute, memberMapping2.TypeDesc, "o");
					if (memberMapping2.CheckSpecified != 0)
					{
						ilg.EndIf();
					}
					if (memberMapping2.CheckShouldPersist)
					{
						ilg.EndIf();
					}
				}
			}
			foreach (MemberMapping memberMapping3 in allMembers)
			{
				if (memberMapping3.Xmlns != null)
				{
					continue;
				}
				CodeIdentifier.CheckValidIdentifier(memberMapping3.Name);
				int num2;
				if (memberMapping3.CheckShouldPersist)
				{
					if (memberMapping3.Elements.Length == 0)
					{
						num2 = ((memberMapping3.Text != null) ? 1 : 0);
						if (num2 == 0)
						{
							goto IL_084d;
						}
					}
					else
					{
						num2 = 1;
					}
					ilg.LdargAddress(arg);
					ilg.Call(memberMapping3.CheckShouldPersistMethodInfo);
					ilg.If();
				}
				else
				{
					num2 = 0;
				}
				goto IL_084d;
				IL_084d:
				if (memberMapping3.CheckSpecified != 0)
				{
					string stringForMember2 = base.RaCodeGen.GetStringForMember("o", memberMapping3.Name + "Specified", mapping.TypeDesc);
					ILGenLoad(stringForMember2);
					ilg.If();
				}
				string choiceSource = null;
				if (memberMapping3.ChoiceIdentifier != null)
				{
					CodeIdentifier.CheckValidIdentifier(memberMapping3.ChoiceIdentifier.MemberName);
					choiceSource = base.RaCodeGen.GetStringForMember("o", memberMapping3.ChoiceIdentifier.MemberName, mapping.TypeDesc);
				}
				WriteMember(base.RaCodeGen.GetSourceForMember("o", memberMapping3, memberMapping3.MemberInfo, mapping.TypeDesc, ilg), choiceSource, memberMapping3.ElementsSortedByDerivation, memberMapping3.Text, memberMapping3.ChoiceIdentifier, memberMapping3.TypeDesc, writeAccessors: true);
				if (memberMapping3.CheckSpecified != 0)
				{
					ilg.EndIf();
				}
				if (num2 != 0)
				{
					ilg.EndIf();
				}
			}
			WriteEndElement("o");
		}
		ilg.EndMethod();
	}

	private bool CanOptimizeWriteListSequence(TypeDesc listElementTypeDesc)
	{
		if (listElementTypeDesc != null)
		{
			return listElementTypeDesc != base.QnameTypeDesc;
		}
		return false;
	}

	private void WriteMember(SourceInfo source, AttributeAccessor attribute, TypeDesc memberTypeDesc, string parent)
	{
		if (memberTypeDesc.IsAbstract)
		{
			return;
		}
		if (memberTypeDesc.IsArrayLike)
		{
			string text = "a" + memberTypeDesc.Name;
			string text2 = "ai" + memberTypeDesc.Name;
			string text3 = "i";
			string cSharpName = memberTypeDesc.CSharpName;
			WriteArrayLocalDecl(cSharpName, text, source, memberTypeDesc);
			if (memberTypeDesc.IsNullable)
			{
				ilg.Ldloc(memberTypeDesc.Type, text);
				ilg.Load(null);
				ilg.If(Cmp.NotEqualTo);
			}
			if (attribute.IsList)
			{
				if (CanOptimizeWriteListSequence(memberTypeDesc.ArrayElementTypeDesc))
				{
					string strVar = ((attribute.Form == XmlSchemaForm.Qualified) ? attribute.Namespace : string.Empty);
					MethodInfo method = typeof(XmlSerializationWriter).GetMethod("get_Writer", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
					MethodInfo method2 = typeof(XmlWriter).GetMethod("WriteStartAttribute", CodeGenerator.InstanceBindingFlags, null, new Type[3]
					{
						typeof(string),
						typeof(string),
						typeof(string)
					}, null);
					ilg.Ldarg(0);
					ilg.Call(method);
					ilg.Load(null);
					ilg.Ldstr(attribute.Name);
					ilg.Ldstr(strVar);
					ilg.Call(method2);
				}
				else
				{
					LocalBuilder local = ilg.DeclareOrGetLocal(typeof(StringBuilder), "sb");
					ConstructorInfo constructor = typeof(StringBuilder).GetConstructor(CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
					ilg.New(constructor);
					ilg.Stloc(local);
				}
			}
			TypeDesc arrayElementTypeDesc = memberTypeDesc.ArrayElementTypeDesc;
			if (memberTypeDesc.IsEnumerable)
			{
				throw CodeGenerator.NotSupported("CDF15337, DDB176069: Also fail in whidbey IEnumerable member with XmlAttributeAttribute");
			}
			if (memberTypeDesc.IsArray)
			{
				LocalBuilder local2 = ilg.DeclareOrGetLocal(typeof(int), text3);
				ilg.For(local2, 0, ilg.GetLocal(text));
			}
			else
			{
				LocalBuilder local3 = ilg.DeclareOrGetLocal(typeof(int), text3);
				ilg.For(local3, 0, ilg.GetLocal(text));
			}
			WriteLocalDecl(text2, base.RaCodeGen.GetStringForArrayMember(text, text3, memberTypeDesc), arrayElementTypeDesc.Type);
			if (attribute.IsList)
			{
				Type returnType = typeof(string);
				string name;
				Type typeFromHandle;
				if (CanOptimizeWriteListSequence(memberTypeDesc.ArrayElementTypeDesc))
				{
					ilg.Ldloc(text3);
					ilg.Ldc(0);
					ilg.If(Cmp.NotEqualTo);
					MethodInfo method3 = typeof(XmlSerializationWriter).GetMethod("get_Writer", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
					MethodInfo method4 = typeof(XmlWriter).GetMethod("WriteString", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(string) }, null);
					ilg.Ldarg(0);
					ilg.Call(method3);
					ilg.Ldstr(" ");
					ilg.Call(method4);
					ilg.EndIf();
					ilg.Ldarg(0);
					name = "WriteValue";
					typeFromHandle = typeof(XmlSerializationWriter);
				}
				else
				{
					MethodInfo method5 = typeof(StringBuilder).GetMethod("Append", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(string) }, null);
					ilg.Ldloc(text3);
					ilg.Ldc(0);
					ilg.If(Cmp.NotEqualTo);
					ilg.Ldloc("sb");
					ilg.Ldstr(" ");
					ilg.Call(method5);
					ilg.Pop();
					ilg.EndIf();
					ilg.Ldloc("sb");
					name = "Append";
					typeFromHandle = typeof(StringBuilder);
				}
				if (attribute.Mapping is EnumMapping)
				{
					WriteEnumValue((EnumMapping)attribute.Mapping, new SourceInfo(text2, text2, null, arrayElementTypeDesc.Type, ilg), out returnType);
				}
				else
				{
					WritePrimitiveValue(arrayElementTypeDesc, new SourceInfo(text2, text2, null, arrayElementTypeDesc.Type, ilg), out returnType);
				}
				MethodInfo method6 = typeFromHandle.GetMethod(name, CodeGenerator.InstanceBindingFlags, null, new Type[1] { returnType }, null);
				ilg.Call(method6);
				if (method6.ReturnType != typeof(void))
				{
					ilg.Pop();
				}
			}
			else
			{
				WriteAttribute(new SourceInfo(text2, text2, null, null, ilg), attribute, parent);
			}
			if (memberTypeDesc.IsEnumerable)
			{
				throw CodeGenerator.NotSupported("CDF15337, DDB176069: Also fail in whidbey IEnumerable member with XmlAttributeAttribute");
			}
			ilg.EndFor();
			if (attribute.IsList)
			{
				if (CanOptimizeWriteListSequence(memberTypeDesc.ArrayElementTypeDesc))
				{
					MethodInfo method7 = typeof(XmlSerializationWriter).GetMethod("get_Writer", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
					MethodInfo method8 = typeof(XmlWriter).GetMethod("WriteEndAttribute", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
					ilg.Ldarg(0);
					ilg.Call(method7);
					ilg.Call(method8);
				}
				else
				{
					MethodInfo method9 = typeof(StringBuilder).GetMethod("get_Length", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
					ilg.Ldloc("sb");
					ilg.Call(method9);
					ilg.Ldc(0);
					ilg.If(Cmp.NotEqualTo);
					List<Type> list = new List<Type>();
					ilg.Ldarg(0);
					ilg.Ldstr(attribute.Name);
					list.Add(typeof(string));
					string text4 = ((attribute.Form == XmlSchemaForm.Qualified) ? attribute.Namespace : string.Empty);
					if (text4 != null)
					{
						ilg.Ldstr(text4);
						list.Add(typeof(string));
					}
					MethodInfo method10 = typeof(object).GetMethod("ToString", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
					ilg.Ldloc("sb");
					ilg.Call(method10);
					list.Add(typeof(string));
					MethodInfo method11 = typeof(XmlSerializationWriter).GetMethod("WriteAttribute", CodeGenerator.InstanceBindingFlags, null, list.ToArray(), null);
					ilg.Call(method11);
					ilg.EndIf();
				}
			}
			if (memberTypeDesc.IsNullable)
			{
				ilg.EndIf();
			}
		}
		else
		{
			WriteAttribute(source, attribute, parent);
		}
	}

	private void WriteAttribute(SourceInfo source, AttributeAccessor attribute, string parent)
	{
		if (attribute.Mapping is SpecialMapping)
		{
			SpecialMapping specialMapping = (SpecialMapping)attribute.Mapping;
			if (specialMapping.TypeDesc.Kind != TypeKind.Attribute && !specialMapping.TypeDesc.CanBeAttributeValue)
			{
				throw new InvalidOperationException(Res.GetString("Internal error."));
			}
			MethodInfo method = typeof(XmlSerializationWriter).GetMethod("WriteXmlAttribute", CodeGenerator.InstanceBindingFlags, null, new Type[2]
			{
				typeof(XmlNode),
				typeof(object)
			}, null);
			ilg.Ldarg(0);
			ilg.Ldloc(source.Source);
			ilg.Ldarg(parent);
			ilg.ConvertValue(ilg.GetArg(parent).ArgType, typeof(object));
			ilg.Call(method);
		}
		else
		{
			TypeDesc typeDesc = attribute.Mapping.TypeDesc;
			source = source.CastTo(typeDesc);
			WritePrimitive("WriteAttribute", attribute.Name, (attribute.Form == XmlSchemaForm.Qualified) ? attribute.Namespace : "", GetConvertedDefaultValue(source.Type, attribute.Default), source, attribute.Mapping, writeXsiType: false, isElement: false, isNullable: false);
		}
	}

	private static object GetConvertedDefaultValue(Type targetType, object rawDefaultValue)
	{
		if (targetType == null)
		{
			return rawDefaultValue;
		}
		if (!targetType.TryConvertTo(rawDefaultValue, out var returnValue))
		{
			return rawDefaultValue;
		}
		return returnValue;
	}

	private void WriteMember(SourceInfo source, string choiceSource, ElementAccessor[] elements, TextAccessor text, ChoiceIdentifierAccessor choice, TypeDesc memberTypeDesc, bool writeAccessors)
	{
		if (memberTypeDesc.IsArrayLike && (elements.Length != 1 || !(elements[0].Mapping is ArrayMapping)))
		{
			WriteArray(source, choiceSource, elements, text, choice, memberTypeDesc);
		}
		else
		{
			WriteElements(source, choiceSource, elements, text, choice, "a" + memberTypeDesc.Name, writeAccessors, memberTypeDesc.IsNullable);
		}
	}

	private void WriteArray(SourceInfo source, string choiceSource, ElementAccessor[] elements, TextAccessor text, ChoiceIdentifierAccessor choice, TypeDesc arrayTypeDesc)
	{
		if (elements.Length != 0 || text != null)
		{
			string cSharpName = arrayTypeDesc.CSharpName;
			string text2 = "a" + arrayTypeDesc.Name;
			WriteArrayLocalDecl(cSharpName, text2, source, arrayTypeDesc);
			LocalBuilder local = ilg.GetLocal(text2);
			if (arrayTypeDesc.IsNullable)
			{
				ilg.Ldloc(local);
				ilg.Load(null);
				ilg.If(Cmp.NotEqualTo);
			}
			string text3 = null;
			if (choice != null)
			{
				string cSharpName2 = choice.Mapping.TypeDesc.CSharpName;
				SourceInfo initValue = new SourceInfo(choiceSource, null, choice.MemberInfo, null, ilg);
				text3 = "c" + choice.Mapping.TypeDesc.Name;
				WriteArrayLocalDecl(cSharpName2 + "[]", text3, initValue, choice.Mapping.TypeDesc);
				Label label = ilg.DefineLabel();
				Label label2 = ilg.DefineLabel();
				LocalBuilder local2 = ilg.GetLocal(text3);
				ilg.Ldloc(local2);
				ilg.Load(null);
				ilg.Beq(label2);
				ilg.Ldloc(local2);
				ilg.Ldlen();
				ilg.Ldloc(local);
				ilg.Ldlen();
				ilg.Clt();
				ilg.Br(label);
				ilg.MarkLabel(label2);
				ilg.Ldc(boolVar: true);
				ilg.MarkLabel(label);
				ilg.If();
				MethodInfo method = typeof(XmlSerializationWriter).GetMethod("CreateInvalidChoiceIdentifierValueException", CodeGenerator.InstanceBindingFlags, null, new Type[2]
				{
					typeof(string),
					typeof(string)
				}, null);
				ilg.Ldarg(0);
				ilg.Ldstr(choice.Mapping.TypeDesc.FullName);
				ilg.Ldstr(choice.MemberName);
				ilg.Call(method);
				ilg.Throw();
				ilg.EndIf();
			}
			WriteArrayItems(elements, text, choice, arrayTypeDesc, text2, text3);
			if (arrayTypeDesc.IsNullable)
			{
				ilg.EndIf();
			}
		}
	}

	private void WriteArrayItems(ElementAccessor[] elements, TextAccessor text, ChoiceIdentifierAccessor choice, TypeDesc arrayTypeDesc, string arrayName, string choiceName)
	{
		TypeDesc arrayElementTypeDesc = arrayTypeDesc.ArrayElementTypeDesc;
		if (arrayTypeDesc.IsEnumerable)
		{
			LocalBuilder localBuilder = ilg.DeclareLocal(typeof(IEnumerator), "e");
			MethodInfo method = arrayTypeDesc.Type.GetMethod("GetEnumerator", CodeGenerator.InstancePublicBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			if (method != null && typeof(IEnumerator).IsAssignableFrom(method.ReturnType))
			{
				ilg.LoadAddress(ilg.GetVariable(arrayName));
			}
			else
			{
				ilg.Load(ilg.GetVariable(arrayName));
				Type type = (arrayTypeDesc.IsGenericInterface ? typeof(IEnumerable<>).MakeGenericType(arrayElementTypeDesc.Type) : typeof(IEnumerable));
				method = type.GetMethod("GetEnumerator", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				ilg.ConvertValue(arrayTypeDesc.Type, type);
			}
			ilg.Call(method);
			ilg.ConvertValue(method.ReturnType, typeof(IEnumerator));
			ilg.Stloc(localBuilder);
			ilg.Ldloc(localBuilder);
			ilg.Load(null);
			ilg.If(Cmp.NotEqualTo);
			ilg.WhileBegin();
			string arrayName2 = arrayName.Replace(arrayTypeDesc.Name, "") + "a" + arrayElementTypeDesc.Name;
			string text2 = arrayName.Replace(arrayTypeDesc.Name, "") + "i" + arrayElementTypeDesc.Name;
			WriteLocalDecl(text2, "e.Current", arrayElementTypeDesc.Type);
			WriteElements(new SourceInfo(text2, null, null, arrayElementTypeDesc.Type, ilg), choiceName + "i", elements, text, choice, arrayName2, writeAccessors: true, isNullable: true);
			ilg.WhileBeginCondition();
			MethodInfo method2 = typeof(IEnumerator).GetMethod("MoveNext", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldloc(localBuilder);
			ilg.Call(method2);
			ilg.WhileEndCondition();
			ilg.WhileEnd();
			ilg.EndIf();
			return;
		}
		string text3 = "i" + arrayName.Replace(arrayTypeDesc.Name, "");
		string arrayName3 = arrayName.Replace(arrayTypeDesc.Name, "") + "a" + arrayElementTypeDesc.Name;
		string text4 = arrayName.Replace(arrayTypeDesc.Name, "") + "i" + arrayElementTypeDesc.Name;
		LocalBuilder local = ilg.DeclareOrGetLocal(typeof(int), text3);
		ilg.For(local, 0, ilg.GetLocal(arrayName));
		if (elements.Length + ((text != null) ? 1 : 0) > 1)
		{
			WriteLocalDecl(text4, base.RaCodeGen.GetStringForArrayMember(arrayName, text3, arrayTypeDesc), arrayElementTypeDesc.Type);
			if (choice != null)
			{
				WriteLocalDecl(choiceName + "i", base.RaCodeGen.GetStringForArrayMember(choiceName, text3, choice.Mapping.TypeDesc), choice.Mapping.TypeDesc.Type);
			}
			WriteElements(new SourceInfo(text4, null, null, arrayElementTypeDesc.Type, ilg), choiceName + "i", elements, text, choice, arrayName3, writeAccessors: true, arrayElementTypeDesc.IsNullable);
		}
		else
		{
			WriteElements(new SourceInfo(base.RaCodeGen.GetStringForArrayMember(arrayName, text3, arrayTypeDesc), null, null, arrayElementTypeDesc.Type, ilg), null, elements, text, choice, arrayName3, writeAccessors: true, arrayElementTypeDesc.IsNullable);
		}
		ilg.EndFor();
	}

	private void WriteElements(SourceInfo source, string enumSource, ElementAccessor[] elements, TextAccessor text, ChoiceIdentifierAccessor choice, string arrayName, bool writeAccessors, bool isNullable)
	{
		if (elements.Length == 0 && text == null)
		{
			return;
		}
		if (elements.Length == 1 && text == null)
		{
			TypeDesc td = (elements[0].IsUnbounded ? elements[0].Mapping.TypeDesc.CreateArrayTypeDesc() : elements[0].Mapping.TypeDesc);
			if (!elements[0].Any && !elements[0].Mapping.TypeDesc.IsOptionalValue)
			{
				source = source.CastTo(td);
			}
			WriteElement(source, elements[0], arrayName, writeAccessors);
			return;
		}
		bool flag = false;
		if (isNullable && choice == null)
		{
			source.Load(typeof(object));
			ilg.Load(null);
			ilg.If(Cmp.NotEqualTo);
			flag = true;
		}
		int num = 0;
		ArrayList arrayList = new ArrayList();
		ElementAccessor elementAccessor = null;
		bool flag2 = false;
		string text2 = choice?.Mapping.TypeDesc.FullName;
		foreach (ElementAccessor elementAccessor2 in elements)
		{
			if (elementAccessor2.Any)
			{
				num++;
				if (elementAccessor2.Name != null && elementAccessor2.Name.Length > 0)
				{
					arrayList.Add(elementAccessor2);
				}
				else if (elementAccessor == null)
				{
					elementAccessor = elementAccessor2;
				}
			}
			else if (choice != null)
			{
				string cSharpName = elementAccessor2.Mapping.TypeDesc.CSharpName;
				object eValue;
				string enumName = text2 + ".@" + FindChoiceEnumValue(elementAccessor2, (EnumMapping)choice.Mapping, out eValue);
				if (flag2)
				{
					ilg.InitElseIf();
				}
				else
				{
					flag2 = true;
					ilg.InitIf();
				}
				ILGenLoad(enumSource, choice?.Mapping.TypeDesc.Type);
				ilg.Load(eValue);
				ilg.Ceq();
				if (isNullable && !elementAccessor2.IsNullable)
				{
					Label label = ilg.DefineLabel();
					Label label2 = ilg.DefineLabel();
					ilg.Brfalse(label);
					source.Load(typeof(object));
					ilg.Load(null);
					ilg.Cne();
					ilg.Br_S(label2);
					ilg.MarkLabel(label);
					ilg.Ldc(boolVar: false);
					ilg.MarkLabel(label2);
				}
				ilg.AndIf();
				WriteChoiceTypeCheck(source, cSharpName, choice, enumName, elementAccessor2.Mapping.TypeDesc);
				SourceInfo sourceInfo = source;
				sourceInfo = source.CastTo(elementAccessor2.Mapping.TypeDesc);
				WriteElement(elementAccessor2.Any ? source : sourceInfo, elementAccessor2, arrayName, writeAccessors);
			}
			else
			{
				TypeDesc typeDesc = (elementAccessor2.IsUnbounded ? elementAccessor2.Mapping.TypeDesc.CreateArrayTypeDesc() : elementAccessor2.Mapping.TypeDesc);
				_ = typeDesc.CSharpName;
				if (flag2)
				{
					ilg.InitElseIf();
				}
				else
				{
					flag2 = true;
					ilg.InitIf();
				}
				WriteInstanceOf(source, typeDesc.Type);
				ilg.AndIf();
				SourceInfo sourceInfo2 = source;
				sourceInfo2 = source.CastTo(typeDesc);
				WriteElement(elementAccessor2.Any ? source : sourceInfo2, elementAccessor2, arrayName, writeAccessors);
			}
		}
		if (flag2 && num > 0 && elements.Length - num <= 0)
		{
			ilg.EndIf();
		}
		if (num > 0)
		{
			if (elements.Length - num > 0)
			{
				ilg.InitElseIf();
			}
			else
			{
				ilg.InitIf();
			}
			_ = typeof(XmlElement).FullName;
			source.Load(typeof(object));
			ilg.IsInst(typeof(XmlElement));
			ilg.Load(null);
			ilg.Cne();
			ilg.AndIf();
			LocalBuilder localBuilder = ilg.DeclareLocal(typeof(XmlElement), "elem");
			source.Load(typeof(XmlElement));
			ilg.Stloc(localBuilder);
			int num2 = 0;
			foreach (ElementAccessor item in arrayList)
			{
				if (num2++ > 0)
				{
					ilg.InitElseIf();
				}
				else
				{
					ilg.InitIf();
				}
				string strVar = null;
				Label label3;
				Label label4;
				if (choice != null)
				{
					strVar = text2 + ".@" + FindChoiceEnumValue(item, (EnumMapping)choice.Mapping, out var eValue2);
					label3 = ilg.DefineLabel();
					label4 = ilg.DefineLabel();
					ILGenLoad(enumSource, choice?.Mapping.TypeDesc.Type);
					ilg.Load(eValue2);
					ilg.Bne(label3);
					if (isNullable && !item.IsNullable)
					{
						source.Load(typeof(object));
						ilg.Load(null);
						ilg.Cne();
					}
					else
					{
						ilg.Ldc(boolVar: true);
					}
					ilg.Br(label4);
					ilg.MarkLabel(label3);
					ilg.Ldc(boolVar: false);
					ilg.MarkLabel(label4);
					ilg.AndIf();
				}
				label3 = ilg.DefineLabel();
				label4 = ilg.DefineLabel();
				MethodInfo method = typeof(XmlNode).GetMethod("get_Name", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				MethodInfo method2 = typeof(XmlNode).GetMethod("get_NamespaceURI", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				ilg.Ldloc(localBuilder);
				ilg.Call(method);
				ilg.Ldstr(item.Name);
				MethodInfo method3 = typeof(string).GetMethod("op_Equality", CodeGenerator.StaticBindingFlags, null, new Type[2]
				{
					typeof(string),
					typeof(string)
				}, null);
				ilg.Call(method3);
				ilg.Brfalse(label3);
				ilg.Ldloc(localBuilder);
				ilg.Call(method2);
				ilg.Ldstr(item.Namespace);
				ilg.Call(method3);
				ilg.Br(label4);
				ilg.MarkLabel(label3);
				ilg.Ldc(boolVar: false);
				ilg.MarkLabel(label4);
				if (choice != null)
				{
					ilg.If();
				}
				else
				{
					ilg.AndIf();
				}
				WriteElement(new SourceInfo("elem", null, null, localBuilder.LocalType, ilg), item, arrayName, writeAccessors);
				if (choice != null)
				{
					ilg.Else();
					MethodInfo method4 = typeof(XmlSerializationWriter).GetMethod("CreateChoiceIdentifierValueException", CodeGenerator.InstanceBindingFlags, null, new Type[4]
					{
						typeof(string),
						typeof(string),
						typeof(string),
						typeof(string)
					}, null);
					ilg.Ldarg(0);
					ilg.Ldstr(strVar);
					ilg.Ldstr(choice.MemberName);
					ilg.Ldloc(localBuilder);
					ilg.Call(method);
					ilg.Ldloc(localBuilder);
					ilg.Call(method2);
					ilg.Call(method4);
					ilg.Throw();
					ilg.EndIf();
				}
			}
			if (num2 > 0)
			{
				ilg.Else();
			}
			if (elementAccessor != null)
			{
				WriteElement(new SourceInfo("elem", null, null, localBuilder.LocalType, ilg), elementAccessor, arrayName, writeAccessors);
			}
			else
			{
				MethodInfo method5 = typeof(XmlSerializationWriter).GetMethod("CreateUnknownAnyElementException", CodeGenerator.InstanceBindingFlags, null, new Type[2]
				{
					typeof(string),
					typeof(string)
				}, null);
				ilg.Ldarg(0);
				ilg.Ldloc(localBuilder);
				MethodInfo method6 = typeof(XmlNode).GetMethod("get_Name", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				MethodInfo method7 = typeof(XmlNode).GetMethod("get_NamespaceURI", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				ilg.Call(method6);
				ilg.Ldloc(localBuilder);
				ilg.Call(method7);
				ilg.Call(method5);
				ilg.Throw();
			}
			if (num2 > 0)
			{
				ilg.EndIf();
			}
		}
		if (text != null)
		{
			_ = text.Mapping.TypeDesc.CSharpName;
			if (elements.Length != 0)
			{
				ilg.InitElseIf();
				WriteInstanceOf(source, text.Mapping.TypeDesc.Type);
				ilg.AndIf();
				SourceInfo source2 = source.CastTo(text.Mapping.TypeDesc);
				WriteText(source2, text);
			}
			else
			{
				SourceInfo source3 = source.CastTo(text.Mapping.TypeDesc);
				WriteText(source3, text);
			}
		}
		if (elements.Length != 0)
		{
			if (isNullable)
			{
				ilg.InitElseIf();
				source.Load(null);
				ilg.Load(null);
				ilg.AndIf(Cmp.NotEqualTo);
			}
			else
			{
				ilg.Else();
			}
			MethodInfo method8 = typeof(XmlSerializationWriter).GetMethod("CreateUnknownTypeException", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(object) }, null);
			ilg.Ldarg(0);
			source.Load(typeof(object));
			ilg.Call(method8);
			ilg.Throw();
			ilg.EndIf();
		}
		if (flag)
		{
			ilg.EndIf();
		}
	}

	private void WriteText(SourceInfo source, TextAccessor text)
	{
		if (text.Mapping is PrimitiveMapping)
		{
			PrimitiveMapping primitiveMapping = (PrimitiveMapping)text.Mapping;
			ilg.Ldarg(0);
			Type returnType;
			if (text.Mapping is EnumMapping)
			{
				WriteEnumValue((EnumMapping)text.Mapping, source, out returnType);
			}
			else
			{
				WritePrimitiveValue(primitiveMapping.TypeDesc, source, out returnType);
			}
			MethodInfo method = typeof(XmlSerializationWriter).GetMethod("WriteValue", CodeGenerator.InstanceBindingFlags, null, new Type[1] { returnType }, null);
			ilg.Call(method);
		}
		else if (text.Mapping is SpecialMapping)
		{
			TypeKind kind = ((SpecialMapping)text.Mapping).TypeDesc.Kind;
			if (kind != TypeKind.Node)
			{
				throw new InvalidOperationException(Res.GetString("Internal error."));
			}
			MethodInfo method2 = source.Type.GetMethod("WriteTo", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(XmlWriter) }, null);
			MethodInfo method3 = typeof(XmlSerializationWriter).GetMethod("get_Writer", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			source.Load(source.Type);
			ilg.Ldarg(0);
			ilg.Call(method3);
			ilg.Call(method2);
		}
	}

	private void WriteElement(SourceInfo source, ElementAccessor element, string arrayName, bool writeAccessor)
	{
		string text = (writeAccessor ? element.Name : element.Mapping.TypeName);
		string text2 = ((element.Any && element.Name.Length == 0) ? null : ((element.Form != XmlSchemaForm.Qualified) ? "" : (writeAccessor ? element.Namespace : element.Mapping.Namespace)));
		if (element.Mapping is NullableMapping)
		{
			if (source.Type == element.Mapping.TypeDesc.Type)
			{
				MethodInfo method = element.Mapping.TypeDesc.Type.GetMethod("get_HasValue", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				source.LoadAddress(element.Mapping.TypeDesc.Type);
				ilg.Call(method);
			}
			else
			{
				source.Load(null);
				ilg.Load(null);
				ilg.Cne();
			}
			ilg.If();
			_ = element.Mapping.TypeDesc.BaseTypeDesc.CSharpName;
			SourceInfo sourceInfo = source.CastTo(element.Mapping.TypeDesc.BaseTypeDesc);
			ElementAccessor elementAccessor = element.Clone();
			elementAccessor.Mapping = ((NullableMapping)element.Mapping).BaseMapping;
			WriteElement(elementAccessor.Any ? source : sourceInfo, elementAccessor, arrayName, writeAccessor);
			if (element.IsNullable)
			{
				ilg.Else();
				WriteLiteralNullTag(element.Name, (element.Form == XmlSchemaForm.Qualified) ? element.Namespace : "");
			}
			ilg.EndIf();
			return;
		}
		if (element.Mapping is ArrayMapping)
		{
			ArrayMapping arrayMapping = (ArrayMapping)element.Mapping;
			if (element.IsUnbounded)
			{
				throw CodeGenerator.NotSupported("Unreachable: IsUnbounded is never set true!");
			}
			ilg.EnterScope();
			string cSharpName = arrayMapping.TypeDesc.CSharpName;
			WriteArrayLocalDecl(cSharpName, arrayName, source, arrayMapping.TypeDesc);
			if (element.IsNullable)
			{
				WriteNullCheckBegin(arrayName, element);
			}
			else if (arrayMapping.TypeDesc.IsNullable)
			{
				ilg.Ldloc(ilg.GetLocal(arrayName));
				ilg.Load(null);
				ilg.If(Cmp.NotEqualTo);
			}
			WriteStartElement(text, text2, writePrefixed: false);
			WriteArrayItems(arrayMapping.ElementsSortedByDerivation, null, null, arrayMapping.TypeDesc, arrayName, null);
			WriteEndElement();
			if (element.IsNullable)
			{
				ilg.EndIf();
			}
			else if (arrayMapping.TypeDesc.IsNullable)
			{
				ilg.EndIf();
			}
			ilg.ExitScope();
			return;
		}
		if (element.Mapping is EnumMapping)
		{
			WritePrimitive("WriteElementString", text, text2, element.Default, source, element.Mapping, writeXsiType: false, isElement: true, element.IsNullable);
			return;
		}
		if (element.Mapping is PrimitiveMapping)
		{
			PrimitiveMapping primitiveMapping = (PrimitiveMapping)element.Mapping;
			if (primitiveMapping.TypeDesc == base.QnameTypeDesc)
			{
				WriteQualifiedNameElement(text, text2, GetConvertedDefaultValue(source.Type, element.Default), source, element.IsNullable, primitiveMapping);
				return;
			}
			string text3 = (primitiveMapping.TypeDesc.XmlEncodingNotRequired ? "Raw" : "");
			WritePrimitive(element.IsNullable ? ("WriteNullableStringLiteral" + text3) : ("WriteElementString" + text3), text, text2, GetConvertedDefaultValue(source.Type, element.Default), source, primitiveMapping, writeXsiType: false, isElement: true, element.IsNullable);
			return;
		}
		if (element.Mapping is StructMapping)
		{
			StructMapping structMapping = (StructMapping)element.Mapping;
			string methodName = ReferenceMapping(structMapping);
			List<Type> list = new List<Type>();
			ilg.Ldarg(0);
			ilg.Ldstr(text);
			list.Add(typeof(string));
			ilg.Ldstr(text2);
			list.Add(typeof(string));
			source.Load(structMapping.TypeDesc.Type);
			list.Add(structMapping.TypeDesc.Type);
			if (structMapping.TypeDesc.IsNullable)
			{
				ilg.Ldc(element.IsNullable);
				list.Add(typeof(bool));
			}
			ilg.Ldc(boolVar: false);
			list.Add(typeof(bool));
			MethodBuilder methodInfo = EnsureMethodBuilder(typeBuilder, methodName, CodeGenerator.PrivateMethodAttributes, typeof(void), list.ToArray());
			ilg.Call(methodInfo);
			return;
		}
		if (element.Mapping is SpecialMapping)
		{
			_ = ((SpecialMapping)element.Mapping).TypeDesc.CSharpName;
			if (element.Mapping is SerializableMapping)
			{
				WriteElementCall("WriteSerializable", typeof(IXmlSerializable), source, text, text2, element.IsNullable, !element.Any);
				return;
			}
			Label label = ilg.DefineLabel();
			Label label2 = ilg.DefineLabel();
			source.Load(null);
			ilg.IsInst(typeof(XmlNode));
			ilg.Brtrue(label);
			source.Load(null);
			ilg.Load(null);
			ilg.Ceq();
			ilg.Br(label2);
			ilg.MarkLabel(label);
			ilg.Ldc(boolVar: true);
			ilg.MarkLabel(label2);
			ilg.If();
			WriteElementCall("WriteElementLiteral", typeof(XmlNode), source, text, text2, element.IsNullable, element.Any);
			ilg.Else();
			MethodInfo method2 = typeof(XmlSerializationWriter).GetMethod("CreateInvalidAnyTypeException", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(object) }, null);
			ilg.Ldarg(0);
			source.Load(null);
			ilg.Call(method2);
			ilg.Throw();
			ilg.EndIf();
			return;
		}
		throw new InvalidOperationException(Res.GetString("Internal error."));
	}

	private void WriteElementCall(string func, Type cast, SourceInfo source, string name, string ns, bool isNullable, bool isAny)
	{
		MethodInfo method = typeof(XmlSerializationWriter).GetMethod(func, CodeGenerator.InstanceBindingFlags, null, new Type[5]
		{
			cast,
			typeof(string),
			typeof(string),
			typeof(bool),
			typeof(bool)
		}, null);
		ilg.Ldarg(0);
		source.Load(cast);
		ilg.Ldstr(name);
		ilg.Ldstr(ns);
		ilg.Ldc(isNullable);
		ilg.Ldc(isAny);
		ilg.Call(method);
	}

	private void WriteCheckDefault(SourceInfo source, object value, bool isNullable)
	{
		if (value is string && ((string)value).Length == 0)
		{
			Label label = ilg.DefineLabel();
			Label label2 = ilg.DefineLabel();
			Label label3 = ilg.DefineLabel();
			source.Load(typeof(string));
			if (isNullable)
			{
				ilg.Brfalse(label3);
			}
			else
			{
				ilg.Brfalse(label2);
			}
			MethodInfo method = typeof(string).GetMethod("get_Length", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			source.Load(typeof(string));
			ilg.Call(method);
			ilg.Ldc(0);
			ilg.Cne();
			ilg.Br(label);
			if (isNullable)
			{
				ilg.MarkLabel(label3);
				ilg.Ldc(boolVar: true);
			}
			else
			{
				ilg.MarkLabel(label2);
				ilg.Ldc(boolVar: false);
			}
			ilg.MarkLabel(label);
			ilg.If();
			return;
		}
		if (value == null)
		{
			source.Load(typeof(object));
			ilg.Load(null);
			ilg.Cne();
		}
		else if (value.GetType().IsPrimitive)
		{
			source.Load(null);
			ilg.Ldc(Convert.ChangeType(value, source.Type, CultureInfo.InvariantCulture));
			ilg.Cne();
		}
		else
		{
			Type type = value.GetType();
			source.Load(type);
			ilg.Ldc(value);
			MethodInfo method2 = type.GetMethod("op_Inequality", CodeGenerator.StaticBindingFlags, null, new Type[2] { type, type }, null);
			if (method2 != null)
			{
				ilg.Call(method2);
			}
			else
			{
				ilg.Cne();
			}
		}
		ilg.If();
	}

	private void WriteChoiceTypeCheck(SourceInfo source, string fullTypeName, ChoiceIdentifierAccessor choice, string enumName, TypeDesc typeDesc)
	{
		Label label = ilg.DefineLabel();
		Label label2 = ilg.DefineLabel();
		source.Load(typeof(object));
		ilg.Load(null);
		ilg.Beq(label);
		WriteInstanceOf(source, typeDesc.Type);
		ilg.Ldc(boolVar: false);
		ilg.Ceq();
		ilg.Br(label2);
		ilg.MarkLabel(label);
		ilg.Ldc(boolVar: false);
		ilg.MarkLabel(label2);
		ilg.If();
		MethodInfo method = typeof(XmlSerializationWriter).GetMethod("CreateMismatchChoiceException", CodeGenerator.InstanceBindingFlags, null, new Type[3]
		{
			typeof(string),
			typeof(string),
			typeof(string)
		}, null);
		ilg.Ldarg(0);
		ilg.Ldstr(typeDesc.FullName);
		ilg.Ldstr(choice.MemberName);
		ilg.Ldstr(enumName);
		ilg.Call(method);
		ilg.Throw();
		ilg.EndIf();
	}

	private void WriteNullCheckBegin(string source, ElementAccessor element)
	{
		LocalBuilder local = ilg.GetLocal(source);
		ilg.Load(local);
		ilg.Load(null);
		ilg.If(Cmp.EqualTo);
		WriteLiteralNullTag(element.Name, (element.Form == XmlSchemaForm.Qualified) ? element.Namespace : "");
		ilg.Else();
	}

	private void WriteNamespaces(string source)
	{
		MethodInfo method = typeof(XmlSerializationWriter).GetMethod("WriteNamespaceDeclarations", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(XmlSerializerNamespaces) }, null);
		ilg.Ldarg(0);
		ILGenLoad(source, typeof(XmlSerializerNamespaces));
		ilg.Call(method);
	}

	private int FindXmlnsIndex(MemberMapping[] members)
	{
		for (int i = 0; i < members.Length; i++)
		{
			if (members[i].Xmlns != null)
			{
				return i;
			}
		}
		return -1;
	}

	private void WriteLocalDecl(string variableName, string initValue, Type type)
	{
		base.RaCodeGen.WriteLocalDecl(variableName, new SourceInfo(initValue, initValue, null, type, ilg));
	}

	private void WriteArrayLocalDecl(string typeName, string variableName, SourceInfo initValue, TypeDesc arrayTypeDesc)
	{
		base.RaCodeGen.WriteArrayLocalDecl(typeName, variableName, initValue, arrayTypeDesc);
	}

	private void WriteTypeCompare(string variable, Type type)
	{
		base.RaCodeGen.WriteTypeCompare(variable, type, ilg);
	}

	private void WriteInstanceOf(SourceInfo source, Type type)
	{
		base.RaCodeGen.WriteInstanceOf(source, type, ilg);
	}

	private void WriteArrayTypeCompare(string variable, Type arrayType)
	{
		base.RaCodeGen.WriteArrayTypeCompare(variable, arrayType, ilg);
	}

	private string FindChoiceEnumValue(ElementAccessor element, EnumMapping choiceMapping, out object eValue)
	{
		string text = null;
		eValue = null;
		for (int i = 0; i < choiceMapping.Constants.Length; i++)
		{
			string xmlName = choiceMapping.Constants[i].XmlName;
			if (element.Any && element.Name.Length == 0)
			{
				if (xmlName == "##any:")
				{
					text = choiceMapping.Constants[i].Name;
					eValue = Enum.ToObject(choiceMapping.TypeDesc.Type, choiceMapping.Constants[i].Value);
					break;
				}
				continue;
			}
			int num = xmlName.LastIndexOf(':');
			string text2 = ((num < 0) ? choiceMapping.Namespace : xmlName.Substring(0, num));
			string text3 = ((num < 0) ? xmlName : xmlName.Substring(num + 1));
			if (element.Name == text3 && ((element.Form == XmlSchemaForm.Unqualified && string.IsNullOrEmpty(text2)) || element.Namespace == text2))
			{
				text = choiceMapping.Constants[i].Name;
				eValue = Enum.ToObject(choiceMapping.TypeDesc.Type, choiceMapping.Constants[i].Value);
				break;
			}
		}
		if (text == null || text.Length == 0)
		{
			if (element.Any && element.Name.Length == 0)
			{
				throw new InvalidOperationException(Res.GetString("Type {0} is missing enumeration value '##any:' corresponding to XmlAnyElementAttribute.", choiceMapping.TypeDesc.FullName));
			}
			throw new InvalidOperationException(Res.GetString("Type {0} is missing enumeration value '{1}' for element '{2}' from namespace '{3}'.", choiceMapping.TypeDesc.FullName, element.Namespace + ":" + element.Name, element.Name, element.Namespace));
		}
		CodeIdentifier.CheckValidIdentifier(text);
		return text;
	}
}
