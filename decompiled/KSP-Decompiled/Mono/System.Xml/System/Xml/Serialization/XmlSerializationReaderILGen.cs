using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.Reflection;
using System.Reflection.Emit;
using System.Security;
using System.Text.RegularExpressions;
using System.Xml.Schema;

namespace System.Xml.Serialization;

internal class XmlSerializationReaderILGen : XmlSerializationILGen
{
	private class CreateCollectionInfo
	{
		private string name;

		private TypeDesc td;

		internal string Name => name;

		internal TypeDesc TypeDesc => td;

		internal CreateCollectionInfo(string name, TypeDesc td)
		{
			this.name = name;
			this.td = td;
		}
	}

	private class Member
	{
		private string source;

		private string arrayName;

		private string arraySource;

		private string choiceArrayName;

		private string choiceSource;

		private string choiceArraySource;

		private MemberMapping mapping;

		private bool isArray;

		private bool isList;

		private bool isNullable;

		private bool multiRef;

		private int fixupIndex = -1;

		private string paramsReadSource;

		private string checkSpecifiedSource;

		internal MemberMapping Mapping => mapping;

		internal string Source => source;

		internal string ArrayName => arrayName;

		internal string ArraySource => arraySource;

		internal bool IsList => isList;

		internal bool IsArrayLike
		{
			get
			{
				if (!isArray)
				{
					return isList;
				}
				return true;
			}
		}

		internal bool IsNullable
		{
			get
			{
				return isNullable;
			}
			set
			{
				isNullable = value;
			}
		}

		internal bool MultiRef
		{
			get
			{
				return multiRef;
			}
			set
			{
				multiRef = value;
			}
		}

		internal int FixupIndex
		{
			get
			{
				return fixupIndex;
			}
			set
			{
				fixupIndex = value;
			}
		}

		internal string ParamsReadSource
		{
			get
			{
				return paramsReadSource;
			}
			set
			{
				paramsReadSource = value;
			}
		}

		internal string CheckSpecifiedSource
		{
			get
			{
				return checkSpecifiedSource;
			}
			set
			{
				checkSpecifiedSource = value;
			}
		}

		internal string ChoiceSource => choiceSource;

		internal string ChoiceArrayName => choiceArrayName;

		internal string ChoiceArraySource => choiceArraySource;

		internal Member(XmlSerializationReaderILGen outerClass, string source, string arrayName, int i, MemberMapping mapping)
			: this(outerClass, source, null, arrayName, i, mapping, multiRef: false, null)
		{
		}

		internal Member(XmlSerializationReaderILGen outerClass, string source, string arrayName, int i, MemberMapping mapping, string choiceSource)
			: this(outerClass, source, null, arrayName, i, mapping, multiRef: false, choiceSource)
		{
		}

		internal Member(XmlSerializationReaderILGen outerClass, string source, string arraySource, string arrayName, int i, MemberMapping mapping)
			: this(outerClass, source, arraySource, arrayName, i, mapping, multiRef: false, null)
		{
		}

		internal Member(XmlSerializationReaderILGen outerClass, string source, string arraySource, string arrayName, int i, MemberMapping mapping, string choiceSource)
			: this(outerClass, source, arraySource, arrayName, i, mapping, multiRef: false, choiceSource)
		{
		}

		internal Member(XmlSerializationReaderILGen outerClass, string source, string arrayName, int i, MemberMapping mapping, bool multiRef)
			: this(outerClass, source, null, arrayName, i, mapping, multiRef, null)
		{
		}

		internal Member(XmlSerializationReaderILGen outerClass, string source, string arraySource, string arrayName, int i, MemberMapping mapping, bool multiRef, string choiceSource)
		{
			this.source = source;
			this.arrayName = arrayName + "_" + i.ToString(CultureInfo.InvariantCulture);
			choiceArrayName = "choice_" + this.arrayName;
			this.choiceSource = choiceSource;
			if (mapping.TypeDesc.IsArrayLike)
			{
				if (arraySource != null)
				{
					this.arraySource = arraySource;
				}
				else
				{
					this.arraySource = outerClass.GetArraySource(mapping.TypeDesc, this.arrayName, multiRef);
				}
				isArray = mapping.TypeDesc.IsArray;
				isList = !isArray;
				if (mapping.ChoiceIdentifier != null)
				{
					choiceArraySource = outerClass.GetArraySource(mapping.TypeDesc, choiceArrayName, multiRef);
					string text = choiceArrayName;
					string text2 = "c" + text;
					string cSharpName = mapping.ChoiceIdentifier.Mapping.TypeDesc.CSharpName;
					string text3 = "(" + cSharpName + "[])";
					string text4 = text + " = " + text3 + "EnsureArrayIndex(" + text + ", " + text2 + ", " + outerClass.RaCodeGen.GetStringForTypeof(cSharpName) + ");";
					choiceArraySource = text4 + outerClass.RaCodeGen.GetStringForArrayMember(text, text2 + "++", mapping.ChoiceIdentifier.Mapping.TypeDesc);
				}
				else
				{
					choiceArraySource = this.choiceSource;
				}
			}
			else
			{
				this.arraySource = ((arraySource == null) ? source : arraySource);
				choiceArraySource = this.choiceSource;
			}
			this.mapping = mapping;
		}
	}

	private Hashtable idNames = new Hashtable();

	private Dictionary<string, FieldBuilder> idNameFields = new Dictionary<string, FieldBuilder>();

	private Hashtable enums;

	private int nextIdNumber;

	private int nextWhileLoopIndex;

	internal Hashtable Enums
	{
		get
		{
			if (enums == null)
			{
				enums = new Hashtable();
			}
			return enums;
		}
	}

	internal XmlSerializationReaderILGen(TypeScope[] scopes, string access, string className)
		: base(scopes, access, className)
	{
	}

	internal void GenerateBegin()
	{
		typeBuilder = CodeGenerator.CreateTypeBuilder(base.ModuleBuilder, base.ClassName, base.TypeAttributes | TypeAttributes.BeforeFieldInit, typeof(XmlSerializationReader), CodeGenerator.EmptyTypeArray);
		TypeScope[] array = base.Scopes;
		foreach (TypeScope typeScope in array)
		{
			foreach (TypeMapping typeMapping in typeScope.TypeMappings)
			{
				if (typeMapping is StructMapping || typeMapping is EnumMapping || typeMapping is NullableMapping)
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
			else if (mapping is NullableMapping)
			{
				WriteNullableMethod((NullableMapping)mapping);
			}
		}
	}

	internal void GenerateEnd(string[] methods, XmlMapping[] xmlMappings, Type[] types)
	{
		GenerateReferencedMethods();
		GenerateInitCallbacksMethod();
		ilg = new CodeGenerator(typeBuilder);
		ilg.BeginMethod(typeof(void), "InitIDs", CodeGenerator.EmptyTypeArray, CodeGenerator.EmptyStringArray, CodeGenerator.ProtectedOverrideMethodAttributes);
		MethodInfo method = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		MethodInfo method2 = typeof(XmlReader).GetMethod("get_NameTable", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		MethodInfo method3 = typeof(XmlNameTable).GetMethod("Add", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(string) }, null);
		foreach (string key in idNames.Keys)
		{
			ilg.Ldarg(0);
			ilg.Ldarg(0);
			ilg.Call(method);
			ilg.Call(method2);
			ilg.Ldstr(key);
			ilg.Call(method3);
			ilg.StoreMember(idNameFields[key]);
		}
		ilg.EndMethod();
		typeBuilder.DefineDefaultConstructor(CodeGenerator.PublicMethodAttributes);
		Type type = typeBuilder.CreateType();
		CreatedTypes.Add(type.Name, type);
	}

	internal string GenerateElement(XmlMapping xmlMapping)
	{
		if (!xmlMapping.IsReadable)
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

	private void WriteIsStartTag(string name, string ns)
	{
		WriteID(name);
		WriteID(ns);
		MethodInfo method = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		MethodInfo method2 = typeof(XmlReader).GetMethod("IsStartElement", CodeGenerator.InstanceBindingFlags, null, new Type[2]
		{
			typeof(string),
			typeof(string)
		}, null);
		ilg.Ldarg(0);
		ilg.Call(method);
		ilg.Ldarg(0);
		ilg.LoadMember(idNameFields[name ?? string.Empty]);
		ilg.Ldarg(0);
		ilg.LoadMember(idNameFields[ns ?? string.Empty]);
		ilg.Call(method2);
		ilg.If();
	}

	private void WriteUnknownNode(string func, string node, ElementAccessor e, bool anyIfs)
	{
		if (anyIfs)
		{
			ilg.Else();
		}
		List<Type> list = new List<Type>();
		ilg.Ldarg(0);
		if (node == "null")
		{
			ilg.Load(null);
		}
		else
		{
			object variable = ilg.GetVariable("p");
			ilg.Load(variable);
			ilg.ConvertValue(ilg.GetVariableType(variable), typeof(object));
		}
		list.Add(typeof(object));
		if (e != null)
		{
			string text = ((e.Form == XmlSchemaForm.Qualified) ? e.Namespace : "");
			text += ":";
			text += e.Name;
			ilg.Ldstr(ReflectionAwareILGen.GetCSharpString(text));
			list.Add(typeof(string));
		}
		MethodInfo method = typeof(XmlSerializationReader).GetMethod(func, CodeGenerator.InstanceBindingFlags, null, list.ToArray(), null);
		ilg.Call(method);
		if (anyIfs)
		{
			ilg.EndIf();
		}
	}

	private void GenerateInitCallbacksMethod()
	{
		ilg = new CodeGenerator(typeBuilder);
		ilg.BeginMethod(typeof(void), "InitCallbacks", CodeGenerator.EmptyTypeArray, CodeGenerator.EmptyStringArray, CodeGenerator.ProtectedOverrideMethodAttributes);
		string methodName = NextMethodName("Array");
		ilg.EndMethod();
		if (false)
		{
			ilg.BeginMethod(typeof(object), GetMethodBuilder(methodName), CodeGenerator.EmptyTypeArray, CodeGenerator.EmptyStringArray, CodeGenerator.PrivateMethodAttributes);
			MethodInfo method = typeof(XmlSerializationReader).GetMethod("UnknownNode", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(object) }, null);
			ilg.Ldarg(0);
			ilg.Load(null);
			ilg.Call(method);
			ilg.Load(null);
			ilg.EndMethod();
		}
	}

	private string GenerateMembersElement(XmlMembersMapping xmlMembersMapping)
	{
		return GenerateLiteralMembersElement(xmlMembersMapping);
	}

	private string GetChoiceIdentifierSource(MemberMapping[] mappings, MemberMapping member)
	{
		string result = null;
		if (member.ChoiceIdentifier != null)
		{
			for (int i = 0; i < mappings.Length; i++)
			{
				if (mappings[i].Name == member.ChoiceIdentifier.MemberName)
				{
					result = "p[" + i.ToString(CultureInfo.InvariantCulture) + "]";
					break;
				}
			}
		}
		return result;
	}

	private string GetChoiceIdentifierSource(MemberMapping mapping, string parent, TypeDesc parentTypeDesc)
	{
		if (mapping.ChoiceIdentifier == null)
		{
			return "";
		}
		CodeIdentifier.CheckValidIdentifier(mapping.ChoiceIdentifier.MemberName);
		return base.RaCodeGen.GetStringForMember(parent, mapping.ChoiceIdentifier.MemberName, parentTypeDesc);
	}

	private string GenerateLiteralMembersElement(XmlMembersMapping xmlMembersMapping)
	{
		ElementAccessor accessor = xmlMembersMapping.Accessor;
		MemberMapping[] members = ((MembersMapping)accessor.Mapping).Members;
		bool hasWrapperElement = ((MembersMapping)accessor.Mapping).HasWrapperElement;
		string text = NextMethodName(accessor.Name);
		ilg = new CodeGenerator(typeBuilder);
		ilg.BeginMethod(typeof(object[]), text, CodeGenerator.EmptyTypeArray, CodeGenerator.EmptyStringArray, CodeGenerator.PublicMethodAttributes);
		ilg.Load(null);
		ilg.Stloc(ilg.ReturnLocal);
		MethodInfo method = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		MethodInfo method2 = typeof(XmlReader).GetMethod("MoveToContent", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg.Ldarg(0);
		ilg.Call(method);
		ilg.Call(method2);
		ilg.Pop();
		LocalBuilder localBuilder = ilg.DeclareLocal(typeof(object[]), "p");
		ilg.NewArray(typeof(object), members.Length);
		ilg.Stloc(localBuilder);
		InitializeValueTypes("p", members);
		int loopIndex = 0;
		if (hasWrapperElement)
		{
			loopIndex = WriteWhileNotLoopStart();
			WriteIsStartTag(accessor.Name, (accessor.Form == XmlSchemaForm.Qualified) ? accessor.Namespace : "");
		}
		Member anyText = null;
		Member anyElement = null;
		Member anyAttribute = null;
		ArrayList arrayList = new ArrayList();
		ArrayList arrayList2 = new ArrayList();
		ArrayList arrayList3 = new ArrayList();
		for (int i = 0; i < members.Length; i++)
		{
			MemberMapping memberMapping = members[i];
			string text2 = "p[" + i.ToString(CultureInfo.InvariantCulture) + "]";
			string arraySource = text2;
			if (memberMapping.Xmlns != null)
			{
				arraySource = "((" + memberMapping.TypeDesc.CSharpName + ")" + text2 + ")";
			}
			string choiceIdentifierSource = GetChoiceIdentifierSource(members, memberMapping);
			Member member = new Member(this, text2, arraySource, "a", i, memberMapping, choiceIdentifierSource);
			Member member2 = new Member(this, text2, null, "a", i, memberMapping, choiceIdentifierSource);
			if (!memberMapping.IsSequence)
			{
				member.ParamsReadSource = "paramsRead[" + i.ToString(CultureInfo.InvariantCulture) + "]";
			}
			if (memberMapping.CheckSpecified == SpecifiedAccessor.ReadWrite)
			{
				string text3 = memberMapping.Name + "Specified";
				for (int j = 0; j < members.Length; j++)
				{
					if (members[j].Name == text3)
					{
						member.CheckSpecifiedSource = "p[" + j.ToString(CultureInfo.InvariantCulture) + "]";
						break;
					}
				}
			}
			bool flag = false;
			if (memberMapping.Text != null)
			{
				anyText = member2;
			}
			if (memberMapping.Attribute != null && memberMapping.Attribute.Any)
			{
				anyAttribute = member2;
			}
			if (memberMapping.Attribute != null || memberMapping.Xmlns != null)
			{
				arrayList3.Add(member);
			}
			else if (memberMapping.Text != null)
			{
				arrayList2.Add(member);
			}
			if (!memberMapping.IsSequence)
			{
				for (int k = 0; k < memberMapping.Elements.Length; k++)
				{
					if (memberMapping.Elements[k].Any && memberMapping.Elements[k].Name.Length == 0)
					{
						anyElement = member2;
						if (memberMapping.Attribute == null && memberMapping.Text == null)
						{
							arrayList2.Add(member2);
						}
						flag = true;
						break;
					}
				}
			}
			if (memberMapping.Attribute != null || memberMapping.Text != null || flag)
			{
				arrayList.Add(member2);
				continue;
			}
			if (memberMapping.TypeDesc.IsArrayLike && (memberMapping.Elements.Length != 1 || !(memberMapping.Elements[0].Mapping is ArrayMapping)))
			{
				arrayList.Add(member2);
				arrayList2.Add(member2);
				continue;
			}
			if (memberMapping.TypeDesc.IsArrayLike && !memberMapping.TypeDesc.IsArray)
			{
				member.ParamsReadSource = null;
			}
			arrayList.Add(member);
		}
		Member[] array = (Member[])arrayList.ToArray(typeof(Member));
		Member[] members2 = (Member[])arrayList2.ToArray(typeof(Member));
		if (array.Length != 0 && array[0].Mapping.IsReturnValue)
		{
			MethodInfo method3 = typeof(XmlSerializationReader).GetMethod("set_IsReturnValue", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(bool) }, null);
			ilg.Ldarg(0);
			ilg.Ldc(boolVar: true);
			ilg.Call(method3);
		}
		WriteParamsRead(members.Length);
		if (arrayList3.Count > 0)
		{
			Member[] members3 = (Member[])arrayList3.ToArray(typeof(Member));
			WriteMemberBegin(members3);
			WriteAttributes(members3, anyAttribute, "UnknownNode", localBuilder);
			WriteMemberEnd(members3);
			MethodInfo method4 = typeof(XmlReader).GetMethod("MoveToElement", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldarg(0);
			ilg.Call(method);
			ilg.Call(method4);
			ilg.Pop();
		}
		WriteMemberBegin(members2);
		if (hasWrapperElement)
		{
			MethodInfo method5 = typeof(XmlReader).GetMethod("get_IsEmptyElement", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldarg(0);
			ilg.Call(method);
			ilg.Call(method5);
			ilg.If();
			MethodInfo method6 = typeof(XmlReader).GetMethod("Skip", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldarg(0);
			ilg.Call(method);
			ilg.Call(method6);
			ilg.Ldarg(0);
			ilg.Call(method);
			ilg.Call(method2);
			ilg.Pop();
			ilg.WhileContinue();
			ilg.EndIf();
			MethodInfo method7 = typeof(XmlReader).GetMethod("ReadStartElement", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldarg(0);
			ilg.Call(method);
			ilg.Call(method7);
		}
		if (IsSequence(array))
		{
			ilg.Ldc(0);
			ilg.Stloc(typeof(int), "state");
		}
		int loopIndex2 = WriteWhileNotLoopStart();
		string text4 = "UnknownNode((object)p, " + ExpectedElements(array) + ");";
		WriteMemberElements(array, text4, text4, anyElement, anyText);
		ilg.Ldarg(0);
		ilg.Call(method);
		ilg.Call(method2);
		ilg.Pop();
		WriteWhileLoopEnd(loopIndex2);
		WriteMemberEnd(members2);
		if (hasWrapperElement)
		{
			MethodInfo method8 = typeof(XmlSerializationReader).GetMethod("ReadEndElement", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldarg(0);
			ilg.Call(method8);
			WriteUnknownNode("UnknownNode", "null", accessor, anyIfs: true);
			ilg.Ldarg(0);
			ilg.Call(method);
			ilg.Call(method2);
			ilg.Pop();
			WriteWhileLoopEnd(loopIndex);
		}
		ilg.Ldloc(ilg.GetLocal("p"));
		ilg.EndMethod();
		return text;
	}

	private void InitializeValueTypes(string arrayName, MemberMapping[] mappings)
	{
		for (int i = 0; i < mappings.Length; i++)
		{
			if (mappings[i].TypeDesc.IsValueType)
			{
				LocalBuilder local = ilg.GetLocal(arrayName);
				ilg.Ldloc(local);
				ilg.Ldc(i);
				base.RaCodeGen.ILGenForCreateInstance(ilg, mappings[i].TypeDesc.Type, ctorInaccessible: false, cast: false);
				ilg.ConvertValue(mappings[i].TypeDesc.Type, typeof(object));
				ilg.Stelem(local.LocalType.GetElementType());
			}
		}
	}

	private string GenerateTypeElement(XmlTypeMapping xmlTypeMapping)
	{
		ElementAccessor accessor = xmlTypeMapping.Accessor;
		TypeMapping mapping = accessor.Mapping;
		string text = NextMethodName(accessor.Name);
		ilg = new CodeGenerator(typeBuilder);
		ilg.BeginMethod(typeof(object), text, CodeGenerator.EmptyTypeArray, CodeGenerator.EmptyStringArray, CodeGenerator.PublicMethodAttributes);
		LocalBuilder localBuilder = ilg.DeclareLocal(typeof(object), "o");
		ilg.Load(null);
		ilg.Stloc(localBuilder);
		MemberMapping memberMapping = new MemberMapping();
		memberMapping.TypeDesc = mapping.TypeDesc;
		memberMapping.Elements = new ElementAccessor[1] { accessor };
		Member[] array = new Member[1]
		{
			new Member(this, "o", "o", "a", 0, memberMapping)
		};
		MethodInfo method = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		MethodInfo method2 = typeof(XmlReader).GetMethod("MoveToContent", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg.Ldarg(0);
		ilg.Call(method);
		ilg.Call(method2);
		ilg.Pop();
		string elseString = "UnknownNode(null, " + ExpectedElements(array) + ");";
		WriteMemberElements(array, "throw CreateUnknownNodeException();", elseString, accessor.Any ? array[0] : null, null);
		ilg.Ldloc(localBuilder);
		ilg.Stloc(ilg.ReturnLocal);
		ilg.Ldloc(ilg.ReturnLocal);
		ilg.EndMethod();
		return text;
	}

	private string NextMethodName(string name)
	{
		return "Read" + (++base.NextMethodNumber).ToString(CultureInfo.InvariantCulture) + "_" + CodeIdentifier.MakeValidInternal(name);
	}

	private string NextIdName(string name)
	{
		int num = ++nextIdNumber;
		return "id" + num.ToString(CultureInfo.InvariantCulture) + "_" + CodeIdentifier.MakeValidInternal(name);
	}

	private void WritePrimitive(TypeMapping mapping, string source)
	{
		if (mapping is EnumMapping)
		{
			string text = ReferenceMapping(mapping);
			if (text == null)
			{
				throw new InvalidOperationException(Res.GetString("The method for enum {0} is missing.", mapping.TypeDesc.Name));
			}
			MethodBuilder methodInfo = EnsureMethodBuilder(typeBuilder, text, CodeGenerator.PrivateMethodAttributes, mapping.TypeDesc.Type, new Type[1] { typeof(string) });
			ilg.Ldarg(0);
			switch (source)
			{
			case "Reader.ReadElementString()":
			case "Reader.ReadString()":
			{
				MethodInfo method3 = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				MethodInfo method4 = typeof(XmlReader).GetMethod((source == "Reader.ReadElementString()") ? "ReadElementString" : "ReadString", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				ilg.Ldarg(0);
				ilg.Call(method3);
				ilg.Call(method4);
				break;
			}
			case "Reader.Value":
			{
				MethodInfo method = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				MethodInfo method2 = typeof(XmlReader).GetMethod("get_Value", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				ilg.Ldarg(0);
				ilg.Call(method);
				ilg.Call(method2);
				break;
			}
			case "vals[i]":
			{
				LocalBuilder local = ilg.GetLocal("vals");
				LocalBuilder local2 = ilg.GetLocal("i");
				ilg.LoadArrayElement(local, local2);
				break;
			}
			case "false":
				ilg.Ldc(boolVar: false);
				break;
			default:
				throw CodeGenerator.NotSupported("Unexpected: " + source);
			}
			ilg.Call(methodInfo);
			return;
		}
		if (mapping.TypeDesc == base.StringTypeDesc)
		{
			switch (source)
			{
			case "Reader.ReadElementString()":
			case "Reader.ReadString()":
			{
				MethodInfo method7 = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				MethodInfo method8 = typeof(XmlReader).GetMethod((source == "Reader.ReadElementString()") ? "ReadElementString" : "ReadString", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				ilg.Ldarg(0);
				ilg.Call(method7);
				ilg.Call(method8);
				break;
			}
			case "Reader.Value":
			{
				MethodInfo method5 = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				MethodInfo method6 = typeof(XmlReader).GetMethod("get_Value", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				ilg.Ldarg(0);
				ilg.Call(method5);
				ilg.Call(method6);
				break;
			}
			case "vals[i]":
			{
				LocalBuilder local3 = ilg.GetLocal("vals");
				LocalBuilder local4 = ilg.GetLocal("i");
				ilg.LoadArrayElement(local3, local4);
				break;
			}
			default:
				throw CodeGenerator.NotSupported("Unexpected: " + source);
			}
			return;
		}
		if (mapping.TypeDesc.FormatterName == "String")
		{
			if (source == "vals[i]")
			{
				if (mapping.TypeDesc.CollapseWhitespace)
				{
					ilg.Ldarg(0);
				}
				LocalBuilder local5 = ilg.GetLocal("vals");
				LocalBuilder local6 = ilg.GetLocal("i");
				ilg.LoadArrayElement(local5, local6);
				if (mapping.TypeDesc.CollapseWhitespace)
				{
					MethodInfo method9 = typeof(XmlSerializationReader).GetMethod("CollapseWhitespace", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(string) }, null);
					ilg.Call(method9);
				}
				return;
			}
			MethodInfo method10 = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			MethodInfo method11 = typeof(XmlReader).GetMethod((source == "Reader.Value") ? "get_Value" : "ReadElementString", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			if (mapping.TypeDesc.CollapseWhitespace)
			{
				ilg.Ldarg(0);
			}
			ilg.Ldarg(0);
			ilg.Call(method10);
			ilg.Call(method11);
			if (mapping.TypeDesc.CollapseWhitespace)
			{
				MethodInfo method12 = typeof(XmlSerializationReader).GetMethod("CollapseWhitespace", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(string) }, null);
				ilg.Call(method12);
			}
			return;
		}
		Type type = ((source == "false") ? typeof(bool) : typeof(string));
		MethodInfo method13;
		if (mapping.TypeDesc.HasCustomFormatter)
		{
			BindingFlags bindingAttr = CodeGenerator.StaticBindingFlags;
			if ((mapping.TypeDesc.FormatterName == "ByteArrayBase64" && source == "false") || (mapping.TypeDesc.FormatterName == "ByteArrayHex" && source == "false") || mapping.TypeDesc.FormatterName == "XmlQualifiedName")
			{
				bindingAttr = CodeGenerator.InstanceBindingFlags;
				ilg.Ldarg(0);
			}
			method13 = typeof(XmlSerializationReader).GetMethod("To" + mapping.TypeDesc.FormatterName, bindingAttr, null, new Type[1] { type }, null);
		}
		else
		{
			method13 = typeof(XmlConvert).GetMethod("To" + mapping.TypeDesc.FormatterName, CodeGenerator.StaticBindingFlags, null, new Type[1] { type }, null);
		}
		switch (source)
		{
		case "Reader.ReadElementString()":
		case "Reader.ReadString()":
		{
			MethodInfo method16 = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			MethodInfo method17 = typeof(XmlReader).GetMethod((source == "Reader.ReadElementString()") ? "ReadElementString" : "ReadString", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldarg(0);
			ilg.Call(method16);
			ilg.Call(method17);
			break;
		}
		case "Reader.Value":
		{
			MethodInfo method14 = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			MethodInfo method15 = typeof(XmlReader).GetMethod("get_Value", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldarg(0);
			ilg.Call(method14);
			ilg.Call(method15);
			break;
		}
		case "vals[i]":
		{
			LocalBuilder local7 = ilg.GetLocal("vals");
			LocalBuilder local8 = ilg.GetLocal("i");
			ilg.LoadArrayElement(local7, local8);
			break;
		}
		default:
			ilg.Ldc(boolVar: false);
			break;
		}
		ilg.Call(method13);
	}

	private string MakeUnique(EnumMapping mapping, string name)
	{
		string text = name;
		object obj = Enums[text];
		if (obj != null)
		{
			if (obj == mapping)
			{
				return null;
			}
			int num = 0;
			while (obj != null)
			{
				num++;
				text = name + num.ToString(CultureInfo.InvariantCulture);
				obj = Enums[text];
			}
		}
		Enums.Add(text, mapping);
		return text;
	}

	private string WriteHashtable(EnumMapping mapping, string typeName, out MethodBuilder get_TableName)
	{
		get_TableName = null;
		CodeIdentifier.CheckValidIdentifier(typeName);
		string text = MakeUnique(mapping, typeName + "Values");
		if (text == null)
		{
			return CodeIdentifier.GetCSharpName(typeName);
		}
		string fieldName = MakeUnique(mapping, "_" + text);
		text = CodeIdentifier.GetCSharpName(text);
		FieldBuilder memberInfo = typeBuilder.DefineField(fieldName, typeof(Hashtable), FieldAttributes.Private);
		PropertyBuilder propertyBuilder = typeBuilder.DefineProperty(text, PropertyAttributes.None, CallingConventions.HasThis, typeof(Hashtable), null, null, null, null, null);
		ilg = new CodeGenerator(typeBuilder);
		ilg.BeginMethod(typeof(Hashtable), "get_" + text, CodeGenerator.EmptyTypeArray, CodeGenerator.EmptyStringArray, MethodAttributes.Assembly | MethodAttributes.HideBySig | MethodAttributes.SpecialName);
		ilg.Ldarg(0);
		ilg.LoadMember(memberInfo);
		ilg.Load(null);
		ilg.If(Cmp.EqualTo);
		ConstructorInfo constructor = typeof(Hashtable).GetConstructor(CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		LocalBuilder localBuilder = ilg.DeclareLocal(typeof(Hashtable), "h");
		ilg.New(constructor);
		ilg.Stloc(localBuilder);
		ConstantMapping[] constants = mapping.Constants;
		MethodInfo method = typeof(Hashtable).GetMethod("Add", CodeGenerator.InstanceBindingFlags, null, new Type[2]
		{
			typeof(object),
			typeof(object)
		}, null);
		for (int i = 0; i < constants.Length; i++)
		{
			ilg.Ldloc(localBuilder);
			ilg.Ldstr(constants[i].XmlName);
			ilg.Ldc(Enum.ToObject(mapping.TypeDesc.Type, constants[i].Value));
			ilg.ConvertValue(mapping.TypeDesc.Type, typeof(long));
			ilg.ConvertValue(typeof(long), typeof(object));
			ilg.Call(method);
		}
		ilg.Ldarg(0);
		ilg.Ldloc(localBuilder);
		ilg.StoreMember(memberInfo);
		ilg.EndIf();
		ilg.Ldarg(0);
		ilg.LoadMember(memberInfo);
		get_TableName = ilg.EndMethod();
		propertyBuilder.SetGetMethod(get_TableName);
		return text;
	}

	private void WriteEnumMethod(EnumMapping mapping)
	{
		MethodBuilder get_TableName = null;
		if (mapping.IsFlags)
		{
			WriteHashtable(mapping, mapping.TypeDesc.Name, out get_TableName);
		}
		string methodName = (string)base.MethodNames[mapping];
		string cSharpName = mapping.TypeDesc.CSharpName;
		List<Type> list = new List<Type>();
		List<string> list2 = new List<string>();
		Type type = mapping.TypeDesc.Type;
		Type underlyingType = Enum.GetUnderlyingType(type);
		list.Add(typeof(string));
		list2.Add("s");
		ilg = new CodeGenerator(typeBuilder);
		ilg.BeginMethod(type, GetMethodBuilder(methodName), list.ToArray(), list2.ToArray(), CodeGenerator.PrivateMethodAttributes);
		ConstantMapping[] constants = mapping.Constants;
		if (mapping.IsFlags)
		{
			MethodInfo method = typeof(XmlSerializationReader).GetMethod("ToEnum", CodeGenerator.StaticBindingFlags, null, new Type[3]
			{
				typeof(string),
				typeof(Hashtable),
				typeof(string)
			}, null);
			ilg.Ldarg("s");
			ilg.Ldarg(0);
			ilg.Call(get_TableName);
			ilg.Ldstr(cSharpName);
			ilg.Call(method);
			if (underlyingType != typeof(long))
			{
				ilg.ConvertValue(typeof(long), underlyingType);
			}
			ilg.Stloc(ilg.ReturnLocal);
			ilg.Br(ilg.ReturnLabel);
		}
		else
		{
			List<Label> list3 = new List<Label>();
			List<object> list4 = new List<object>();
			Label label = ilg.DefineLabel();
			Label label2 = ilg.DefineLabel();
			LocalBuilder tempLocal = ilg.GetTempLocal(typeof(string));
			ilg.Ldarg("s");
			ilg.Stloc(tempLocal);
			ilg.Ldloc(tempLocal);
			ilg.Brfalse(label);
			Hashtable hashtable = new Hashtable();
			foreach (ConstantMapping constantMapping in constants)
			{
				CodeIdentifier.CheckValidIdentifier(constantMapping.Name);
				if (hashtable[constantMapping.XmlName] == null)
				{
					hashtable[constantMapping.XmlName] = constantMapping.XmlName;
					Label label3 = ilg.DefineLabel();
					ilg.Ldloc(tempLocal);
					ilg.Ldstr(constantMapping.XmlName);
					MethodInfo method2 = typeof(string).GetMethod("op_Equality", CodeGenerator.StaticBindingFlags, null, new Type[2]
					{
						typeof(string),
						typeof(string)
					}, null);
					ilg.Call(method2);
					ilg.Brtrue(label3);
					list3.Add(label3);
					list4.Add(Enum.ToObject(mapping.TypeDesc.Type, constantMapping.Value));
				}
			}
			ilg.Br(label);
			for (int j = 0; j < list3.Count; j++)
			{
				ilg.MarkLabel(list3[j]);
				ilg.Ldc(list4[j]);
				ilg.Stloc(ilg.ReturnLocal);
				ilg.Br(ilg.ReturnLabel);
			}
			MethodInfo method3 = typeof(XmlSerializationReader).GetMethod("CreateUnknownConstantException", CodeGenerator.InstanceBindingFlags, null, new Type[2]
			{
				typeof(string),
				typeof(Type)
			}, null);
			ilg.MarkLabel(label);
			ilg.Ldarg(0);
			ilg.Ldarg("s");
			ilg.Ldc(mapping.TypeDesc.Type);
			ilg.Call(method3);
			ilg.Throw();
			ilg.MarkLabel(label2);
		}
		ilg.MarkLabel(ilg.ReturnLabel);
		ilg.Ldloc(ilg.ReturnLocal);
		ilg.EndMethod();
	}

	private void WriteDerivedTypes(StructMapping mapping, bool isTypedReturn, string returnTypeName)
	{
		for (StructMapping structMapping = mapping.DerivedMappings; structMapping != null; structMapping = structMapping.NextDerivedMapping)
		{
			ilg.InitElseIf();
			WriteQNameEqual("xsiType", structMapping.TypeName, structMapping.Namespace);
			ilg.AndIf();
			string methodName = ReferenceMapping(structMapping);
			List<Type> list = new List<Type>();
			ilg.Ldarg(0);
			if (structMapping.TypeDesc.IsNullable)
			{
				ilg.Ldarg("isNullable");
				list.Add(typeof(bool));
			}
			ilg.Ldc(boolVar: false);
			list.Add(typeof(bool));
			MethodBuilder methodBuilder = EnsureMethodBuilder(typeBuilder, methodName, CodeGenerator.PrivateMethodAttributes, structMapping.TypeDesc.Type, list.ToArray());
			ilg.Call(methodBuilder);
			ilg.ConvertValue(methodBuilder.ReturnType, ilg.ReturnLocal.LocalType);
			ilg.Stloc(ilg.ReturnLocal);
			ilg.Br(ilg.ReturnLabel);
			WriteDerivedTypes(structMapping, isTypedReturn, returnTypeName);
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
					WriteQNameEqual("xsiType", enumMapping.TypeName, enumMapping.Namespace);
					ilg.AndIf();
					MethodInfo method = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
					MethodInfo method2 = typeof(XmlReader).GetMethod("ReadStartElement", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
					ilg.Ldarg(0);
					ilg.Call(method);
					ilg.Call(method2);
					string methodName = ReferenceMapping(enumMapping);
					LocalBuilder localBuilder = ilg.DeclareOrGetLocal(typeof(object), "e");
					MethodBuilder methodBuilder = EnsureMethodBuilder(typeBuilder, methodName, CodeGenerator.PrivateMethodAttributes, enumMapping.TypeDesc.Type, new Type[1] { typeof(string) });
					MethodInfo method3 = typeof(XmlSerializationReader).GetMethod("CollapseWhitespace", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(string) }, null);
					MethodInfo method4 = typeof(XmlReader).GetMethod("ReadString", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
					ilg.Ldarg(0);
					ilg.Ldarg(0);
					ilg.Ldarg(0);
					ilg.Call(method);
					ilg.Call(method4);
					ilg.Call(method3);
					ilg.Call(methodBuilder);
					ilg.ConvertValue(methodBuilder.ReturnType, localBuilder.LocalType);
					ilg.Stloc(localBuilder);
					MethodInfo method5 = typeof(XmlSerializationReader).GetMethod("ReadEndElement", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
					ilg.Ldarg(0);
					ilg.Call(method5);
					ilg.Ldloc(localBuilder);
					ilg.Stloc(ilg.ReturnLocal);
					ilg.Br(ilg.ReturnLabel);
				}
				else
				{
					if (!(typeMapping is ArrayMapping))
					{
						continue;
					}
					ArrayMapping arrayMapping = (ArrayMapping)typeMapping;
					if (arrayMapping.TypeDesc.HasDefaultConstructor)
					{
						ilg.InitElseIf();
						WriteQNameEqual("xsiType", arrayMapping.TypeName, arrayMapping.Namespace);
						ilg.AndIf();
						ilg.EnterScope();
						MemberMapping memberMapping = new MemberMapping();
						memberMapping.TypeDesc = arrayMapping.TypeDesc;
						memberMapping.Elements = arrayMapping.Elements;
						string text = "a";
						string arrayName = "z";
						Member member = new Member(this, text, arrayName, 0, memberMapping);
						TypeDesc typeDesc = arrayMapping.TypeDesc;
						LocalBuilder localBuilder2 = ilg.DeclareLocal(arrayMapping.TypeDesc.Type, text);
						if (arrayMapping.TypeDesc.IsValueType)
						{
							base.RaCodeGen.ILGenForCreateInstance(ilg, typeDesc.Type, ctorInaccessible: false, cast: false);
						}
						else
						{
							ilg.Load(null);
						}
						ilg.Stloc(localBuilder2);
						WriteArray(member.Source, member.ArrayName, arrayMapping, readOnly: false, isNullable: false, -1, 0);
						ilg.Ldloc(localBuilder2);
						ilg.Stloc(ilg.ReturnLocal);
						ilg.Br(ilg.ReturnLabel);
						ilg.ExitScope();
					}
				}
			}
		}
	}

	private void WriteNullableMethod(NullableMapping nullableMapping)
	{
		string methodName = (string)base.MethodNames[nullableMapping];
		ilg = new CodeGenerator(typeBuilder);
		ilg.BeginMethod(nullableMapping.TypeDesc.Type, GetMethodBuilder(methodName), new Type[1] { typeof(bool) }, new string[1] { "checkType" }, CodeGenerator.PrivateMethodAttributes);
		LocalBuilder localBuilder = ilg.DeclareLocal(nullableMapping.TypeDesc.Type, "o");
		ilg.LoadAddress(localBuilder);
		ilg.InitObj(nullableMapping.TypeDesc.Type);
		MethodInfo method = typeof(XmlSerializationReader).GetMethod("ReadNull", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg.Ldarg(0);
		ilg.Call(method);
		ilg.If();
		ilg.Ldloc(localBuilder);
		ilg.Stloc(ilg.ReturnLocal);
		ilg.Br(ilg.ReturnLabel);
		ilg.EndIf();
		ElementAccessor elementAccessor = new ElementAccessor();
		elementAccessor.Mapping = nullableMapping.BaseMapping;
		elementAccessor.Any = false;
		elementAccessor.IsNullable = nullableMapping.BaseMapping.TypeDesc.IsNullable;
		WriteElement("o", null, null, elementAccessor, null, null, checkForNull: false, readOnly: false, -1, -1);
		ilg.Ldloc(localBuilder);
		ilg.Stloc(ilg.ReturnLocal);
		ilg.Br(ilg.ReturnLabel);
		ilg.MarkLabel(ilg.ReturnLabel);
		ilg.Ldloc(ilg.ReturnLocal);
		ilg.EndMethod();
	}

	private void WriteStructMethod(StructMapping structMapping)
	{
		WriteLiteralStructMethod(structMapping);
	}

	private void WriteLiteralStructMethod(StructMapping structMapping)
	{
		string methodName = (string)base.MethodNames[structMapping];
		string cSharpName = structMapping.TypeDesc.CSharpName;
		ilg = new CodeGenerator(typeBuilder);
		List<Type> list = new List<Type>();
		List<string> list2 = new List<string>();
		if (structMapping.TypeDesc.IsNullable)
		{
			list.Add(typeof(bool));
			list2.Add("isNullable");
		}
		list.Add(typeof(bool));
		list2.Add("checkType");
		ilg.BeginMethod(structMapping.TypeDesc.Type, GetMethodBuilder(methodName), list.ToArray(), list2.ToArray(), CodeGenerator.PrivateMethodAttributes);
		LocalBuilder localBuilder = ilg.DeclareLocal(typeof(XmlQualifiedName), "xsiType");
		LocalBuilder localBuilder2 = ilg.DeclareLocal(typeof(bool), "isNull");
		MethodInfo method = typeof(XmlSerializationReader).GetMethod("GetXsiType", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		MethodInfo method2 = typeof(XmlSerializationReader).GetMethod("ReadNull", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		Label label = ilg.DefineLabel();
		Label label2 = ilg.DefineLabel();
		ilg.Ldarg("checkType");
		ilg.Brtrue(label);
		ilg.Load(null);
		ilg.Br_S(label2);
		ilg.MarkLabel(label);
		ilg.Ldarg(0);
		ilg.Call(method);
		ilg.MarkLabel(label2);
		ilg.Stloc(localBuilder);
		ilg.Ldc(boolVar: false);
		ilg.Stloc(localBuilder2);
		if (structMapping.TypeDesc.IsNullable)
		{
			ilg.Ldarg("isNullable");
			ilg.If();
			ilg.Ldarg(0);
			ilg.Call(method2);
			ilg.Stloc(localBuilder2);
			ilg.EndIf();
		}
		ilg.Ldarg("checkType");
		ilg.If();
		if (structMapping.TypeDesc.IsRoot)
		{
			ilg.Ldloc(localBuilder2);
			ilg.If();
			ilg.Ldloc(localBuilder);
			ilg.Load(null);
			ilg.If(Cmp.NotEqualTo);
			MethodInfo method3 = typeof(XmlSerializationReader).GetMethod("ReadTypedNull", CodeGenerator.InstanceBindingFlags, null, new Type[1] { localBuilder.LocalType }, null);
			ilg.Ldarg(0);
			ilg.Ldloc(localBuilder);
			ilg.Call(method3);
			ilg.Stloc(ilg.ReturnLocal);
			ilg.Br(ilg.ReturnLabel);
			ilg.Else();
			if (structMapping.TypeDesc.IsValueType)
			{
				throw CodeGenerator.NotSupported("Arg_NeverValueType");
			}
			ilg.Load(null);
			ilg.Stloc(ilg.ReturnLocal);
			ilg.Br(ilg.ReturnLabel);
			ilg.EndIf();
			ilg.EndIf();
		}
		ilg.Ldloc(typeof(XmlQualifiedName), "xsiType");
		ilg.Load(null);
		ilg.Ceq();
		if (!structMapping.TypeDesc.IsRoot)
		{
			label = ilg.DefineLabel();
			label2 = ilg.DefineLabel();
			ilg.Brtrue(label);
			WriteQNameEqual("xsiType", structMapping.TypeName, structMapping.Namespace);
			ilg.Br_S(label2);
			ilg.MarkLabel(label);
			ilg.Ldc(boolVar: true);
			ilg.MarkLabel(label2);
		}
		ilg.If();
		if (structMapping.TypeDesc.IsRoot)
		{
			ConstructorInfo constructor = typeof(XmlQualifiedName).GetConstructor(CodeGenerator.InstanceBindingFlags, null, new Type[2]
			{
				typeof(string),
				typeof(string)
			}, null);
			MethodInfo method4 = typeof(XmlSerializationReader).GetMethod("ReadTypedPrimitive", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(XmlQualifiedName) }, null);
			ilg.Ldarg(0);
			ilg.Ldstr("anyType");
			ilg.Ldstr("http://www.w3.org/2001/XMLSchema");
			ilg.New(constructor);
			ilg.Call(method4);
			ilg.Stloc(ilg.ReturnLocal);
			ilg.Br(ilg.ReturnLabel);
		}
		WriteDerivedTypes(structMapping, !structMapping.TypeDesc.IsRoot, cSharpName);
		if (structMapping.TypeDesc.IsRoot)
		{
			WriteEnumAndArrayTypes();
		}
		ilg.Else();
		if (structMapping.TypeDesc.IsRoot)
		{
			MethodInfo method5 = typeof(XmlSerializationReader).GetMethod("ReadTypedPrimitive", CodeGenerator.InstanceBindingFlags, null, new Type[1] { localBuilder.LocalType }, null);
			ilg.Ldarg(0);
			ilg.Ldloc(localBuilder);
			ilg.Call(method5);
			ilg.Stloc(ilg.ReturnLocal);
			ilg.Br(ilg.ReturnLabel);
		}
		else
		{
			MethodInfo method6 = typeof(XmlSerializationReader).GetMethod("CreateUnknownTypeException", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(XmlQualifiedName) }, null);
			ilg.Ldarg(0);
			ilg.Ldloc(localBuilder);
			ilg.Call(method6);
			ilg.Throw();
		}
		ilg.EndIf();
		ilg.EndIf();
		if (structMapping.TypeDesc.IsNullable)
		{
			ilg.Ldloc(typeof(bool), "isNull");
			ilg.If();
			ilg.Load(null);
			ilg.Stloc(ilg.ReturnLocal);
			ilg.Br(ilg.ReturnLabel);
			ilg.EndIf();
		}
		if (structMapping.TypeDesc.IsAbstract)
		{
			MethodInfo method7 = typeof(XmlSerializationReader).GetMethod("CreateAbstractTypeException", CodeGenerator.InstanceBindingFlags, null, new Type[2]
			{
				typeof(string),
				typeof(string)
			}, null);
			ilg.Ldarg(0);
			ilg.Ldstr(structMapping.TypeName);
			ilg.Ldstr(structMapping.Namespace);
			ilg.Call(method7);
			ilg.Throw();
		}
		else
		{
			if (structMapping.TypeDesc.Type != null && typeof(XmlSchemaObject).IsAssignableFrom(structMapping.TypeDesc.Type))
			{
				MethodInfo method8 = typeof(XmlSerializationReader).GetMethod("set_DecodeName", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(bool) }, null);
				ilg.Ldarg(0);
				ilg.Ldc(boolVar: false);
				ilg.Call(method8);
			}
			WriteCreateMapping(structMapping, "o");
			LocalBuilder local = ilg.GetLocal("o");
			MemberMapping[] settableMembers = TypeScope.GetSettableMembers(structMapping, memberInfos);
			Member member = null;
			Member member2 = null;
			Member member3 = null;
			bool flag = structMapping.HasExplicitSequence();
			ArrayList arrayList = new ArrayList(settableMembers.Length);
			ArrayList arrayList2 = new ArrayList(settableMembers.Length);
			ArrayList arrayList3 = new ArrayList(settableMembers.Length);
			for (int i = 0; i < settableMembers.Length; i++)
			{
				MemberMapping memberMapping = settableMembers[i];
				CodeIdentifier.CheckValidIdentifier(memberMapping.Name);
				string stringForMember = base.RaCodeGen.GetStringForMember("o", memberMapping.Name, structMapping.TypeDesc);
				Member member4 = new Member(this, stringForMember, "a", i, memberMapping, GetChoiceIdentifierSource(memberMapping, "o", structMapping.TypeDesc));
				if (!memberMapping.IsSequence)
				{
					member4.ParamsReadSource = "paramsRead[" + i.ToString(CultureInfo.InvariantCulture) + "]";
				}
				member4.IsNullable = memberMapping.TypeDesc.IsNullable;
				if (memberMapping.CheckSpecified == SpecifiedAccessor.ReadWrite)
				{
					member4.CheckSpecifiedSource = base.RaCodeGen.GetStringForMember("o", memberMapping.Name + "Specified", structMapping.TypeDesc);
				}
				if (memberMapping.Text != null)
				{
					member = member4;
				}
				if (memberMapping.Attribute != null && memberMapping.Attribute.Any)
				{
					member3 = member4;
				}
				if (!flag)
				{
					for (int j = 0; j < memberMapping.Elements.Length; j++)
					{
						if (memberMapping.Elements[j].Any && (memberMapping.Elements[j].Name == null || memberMapping.Elements[j].Name.Length == 0))
						{
							member2 = member4;
							break;
						}
					}
				}
				else if (memberMapping.IsParticle && !memberMapping.IsSequence)
				{
					structMapping.FindDeclaringMapping(memberMapping, out var declaringMapping, structMapping.TypeName);
					throw new InvalidOperationException(Res.GetString("There was an error processing type '{0}'. Type member '{1}' declared in '{2}' is missing required '{3}' property. If one class in the class hierarchy uses explicit sequencing feature ({3}), then its base class and all derived classes have to do the same.", structMapping.TypeDesc.FullName, memberMapping.Name, declaringMapping.TypeDesc.FullName, "Order"));
				}
				if (memberMapping.Attribute == null && memberMapping.Elements.Length == 1 && memberMapping.Elements[0].Mapping is ArrayMapping)
				{
					Member member5 = new Member(this, stringForMember, stringForMember, "a", i, memberMapping, GetChoiceIdentifierSource(memberMapping, "o", structMapping.TypeDesc));
					member5.CheckSpecifiedSource = member4.CheckSpecifiedSource;
					arrayList3.Add(member5);
				}
				else
				{
					arrayList3.Add(member4);
				}
				if (!memberMapping.TypeDesc.IsArrayLike)
				{
					continue;
				}
				arrayList.Add(member4);
				if (memberMapping.TypeDesc.IsArrayLike && (memberMapping.Elements.Length != 1 || !(memberMapping.Elements[0].Mapping is ArrayMapping)))
				{
					member4.ParamsReadSource = null;
					if (member4 != member && member4 != member2)
					{
						arrayList2.Add(member4);
					}
				}
				else if (!memberMapping.TypeDesc.IsArray)
				{
					member4.ParamsReadSource = null;
				}
			}
			if (member2 != null)
			{
				arrayList2.Add(member2);
			}
			if (member != null && member != member2)
			{
				arrayList2.Add(member);
			}
			Member[] members = (Member[])arrayList.ToArray(typeof(Member));
			Member[] members2 = (Member[])arrayList2.ToArray(typeof(Member));
			Member[] members3 = (Member[])arrayList3.ToArray(typeof(Member));
			WriteMemberBegin(members);
			WriteParamsRead(settableMembers.Length);
			WriteAttributes(members3, member3, "UnknownNode", local);
			if (member3 != null)
			{
				WriteMemberEnd(members);
			}
			MethodInfo method9 = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			MethodInfo method10 = typeof(XmlReader).GetMethod("MoveToElement", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldarg(0);
			ilg.Call(method9);
			ilg.Call(method10);
			ilg.Pop();
			MethodInfo method11 = typeof(XmlReader).GetMethod("get_IsEmptyElement", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldarg(0);
			ilg.Call(method9);
			ilg.Call(method11);
			ilg.If();
			MethodInfo method12 = typeof(XmlReader).GetMethod("Skip", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldarg(0);
			ilg.Call(method9);
			ilg.Call(method12);
			WriteMemberEnd(members2);
			ilg.Ldloc(local);
			ilg.Stloc(ilg.ReturnLocal);
			ilg.Br(ilg.ReturnLabel);
			ilg.EndIf();
			MethodInfo method13 = typeof(XmlReader).GetMethod("ReadStartElement", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldarg(0);
			ilg.Call(method9);
			ilg.Call(method13);
			if (IsSequence(members3))
			{
				ilg.Ldc(0);
				ilg.Stloc(typeof(int), "state");
			}
			int loopIndex = WriteWhileNotLoopStart();
			string text = "UnknownNode((object)o, " + ExpectedElements(members3) + ");";
			WriteMemberElements(members3, text, text, member2, member);
			MethodInfo method14 = typeof(XmlReader).GetMethod("MoveToContent", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldarg(0);
			ilg.Call(method9);
			ilg.Call(method14);
			ilg.Pop();
			WriteWhileLoopEnd(loopIndex);
			WriteMemberEnd(members2);
			MethodInfo method15 = typeof(XmlSerializationReader).GetMethod("ReadEndElement", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldarg(0);
			ilg.Call(method15);
			ilg.Ldloc(structMapping.TypeDesc.Type, "o");
			ilg.Stloc(ilg.ReturnLocal);
		}
		ilg.MarkLabel(ilg.ReturnLabel);
		ilg.Ldloc(ilg.ReturnLocal);
		ilg.EndMethod();
	}

	private void WriteQNameEqual(string source, string name, string ns)
	{
		WriteID(name);
		WriteID(ns);
		MethodInfo method = typeof(XmlQualifiedName).GetMethod("get_Name", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		MethodInfo method2 = typeof(XmlQualifiedName).GetMethod("get_Namespace", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		Label label = ilg.DefineLabel();
		Label label2 = ilg.DefineLabel();
		LocalBuilder local = ilg.GetLocal(source);
		ilg.Ldloc(local);
		ilg.Call(method);
		ilg.Ldarg(0);
		ilg.LoadMember(idNameFields[name ?? string.Empty]);
		ilg.Bne(label2);
		ilg.Ldloc(local);
		ilg.Call(method2);
		ilg.Ldarg(0);
		ilg.LoadMember(idNameFields[ns ?? string.Empty]);
		ilg.Ceq();
		ilg.Br_S(label);
		ilg.MarkLabel(label2);
		ilg.Ldc(boolVar: false);
		ilg.MarkLabel(label);
	}

	private void WriteXmlNodeEqual(string source, string name, string ns)
	{
		WriteXmlNodeEqual(source, name, ns, doAndIf: true);
	}

	private void WriteXmlNodeEqual(string source, string name, string ns, bool doAndIf)
	{
		bool num = string.IsNullOrEmpty(name);
		if (!num)
		{
			WriteID(name);
		}
		WriteID(ns);
		MethodInfo method = typeof(XmlSerializationReader).GetMethod("get_" + source, CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		MethodInfo method2 = typeof(XmlReader).GetMethod("get_LocalName", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		MethodInfo method3 = typeof(XmlReader).GetMethod("get_NamespaceURI", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		Label label = ilg.DefineLabel();
		Label label2 = ilg.DefineLabel();
		if (!num)
		{
			ilg.Ldarg(0);
			ilg.Call(method);
			ilg.Call(method2);
			ilg.Ldarg(0);
			ilg.LoadMember(idNameFields[name ?? string.Empty]);
			ilg.Bne(label);
		}
		ilg.Ldarg(0);
		ilg.Call(method);
		ilg.Call(method3);
		ilg.Ldarg(0);
		ilg.LoadMember(idNameFields[ns ?? string.Empty]);
		ilg.Ceq();
		if (!num)
		{
			ilg.Br_S(label2);
			ilg.MarkLabel(label);
			ilg.Ldc(boolVar: false);
			ilg.MarkLabel(label2);
		}
		if (doAndIf)
		{
			ilg.AndIf();
		}
	}

	private void WriteID(string name)
	{
		if (name == null)
		{
			name = "";
		}
		string text = (string)idNames[name];
		if (text == null)
		{
			text = NextIdName(name);
			idNames.Add(name, text);
			idNameFields.Add(name, typeBuilder.DefineField(text, typeof(string), FieldAttributes.Private));
		}
	}

	private void WriteAttributes(Member[] members, Member anyAttribute, string elseCall, LocalBuilder firstParam)
	{
		int num = 0;
		Member member = null;
		ArrayList arrayList = new ArrayList();
		MethodInfo method = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		MethodInfo method2 = typeof(XmlReader).GetMethod("MoveToNextAttribute", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg.WhileBegin();
		foreach (Member member2 in members)
		{
			if (member2.Mapping.Xmlns != null)
			{
				member = member2;
			}
			else
			{
				if (member2.Mapping.Ignore)
				{
					continue;
				}
				AttributeAccessor attribute = member2.Mapping.Attribute;
				if (attribute != null && !attribute.Any)
				{
					arrayList.Add(attribute);
					if (num++ > 0)
					{
						ilg.InitElseIf();
					}
					else
					{
						ilg.InitIf();
					}
					if (member2.ParamsReadSource != null)
					{
						ILGenParamsReadSource(member2.ParamsReadSource);
						ilg.Ldc(boolVar: false);
						ilg.AndIf(Cmp.EqualTo);
					}
					if (attribute.IsSpecialXmlNamespace)
					{
						WriteXmlNodeEqual("Reader", attribute.Name, "http://www.w3.org/XML/1998/namespace");
					}
					else
					{
						WriteXmlNodeEqual("Reader", attribute.Name, (attribute.Form == XmlSchemaForm.Qualified) ? attribute.Namespace : "");
					}
					WriteAttribute(member2);
				}
			}
		}
		if (num > 0)
		{
			ilg.InitElseIf();
		}
		else
		{
			ilg.InitIf();
		}
		if (member != null)
		{
			MethodInfo method3 = typeof(XmlSerializationReader).GetMethod("IsXmlnsAttribute", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(string) }, null);
			MethodInfo method4 = typeof(XmlReader).GetMethod("get_Name", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			MethodInfo method5 = typeof(XmlReader).GetMethod("get_LocalName", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			MethodInfo method6 = typeof(XmlReader).GetMethod("get_Value", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldarg(0);
			ilg.Ldarg(0);
			ilg.Call(method);
			ilg.Call(method4);
			ilg.Call(method3);
			ilg.Ldc(boolVar: true);
			ilg.AndIf(Cmp.EqualTo);
			ILGenLoad(member.Source);
			ilg.Load(null);
			ilg.If(Cmp.EqualTo);
			WriteSourceBegin(member.Source);
			ConstructorInfo constructor = member.Mapping.TypeDesc.Type.GetConstructor(CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.New(constructor);
			WriteSourceEnd(member.Source, member.Mapping.TypeDesc.Type);
			ilg.EndIf();
			Label label = ilg.DefineLabel();
			Label label2 = ilg.DefineLabel();
			MethodInfo method7 = member.Mapping.TypeDesc.Type.GetMethod("Add", CodeGenerator.InstanceBindingFlags, null, new Type[2]
			{
				typeof(string),
				typeof(string)
			}, null);
			MethodInfo method8 = typeof(string).GetMethod("get_Length", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ILGenLoad(member.ArraySource, member.Mapping.TypeDesc.Type);
			ilg.Ldarg(0);
			ilg.Call(method);
			ilg.Call(method4);
			ilg.Call(method8);
			ilg.Ldc(5);
			ilg.Beq(label);
			ilg.Ldarg(0);
			ilg.Call(method);
			ilg.Call(method5);
			ilg.Br(label2);
			ilg.MarkLabel(label);
			ilg.Ldstr(string.Empty);
			ilg.MarkLabel(label2);
			ilg.Ldarg(0);
			ilg.Call(method);
			ilg.Call(method6);
			ilg.Call(method7);
			ilg.Else();
		}
		else
		{
			MethodInfo method9 = typeof(XmlSerializationReader).GetMethod("IsXmlnsAttribute", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(string) }, null);
			MethodInfo method10 = typeof(XmlReader).GetMethod("get_Name", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldarg(0);
			ilg.Ldarg(0);
			ilg.Call(method);
			ilg.Call(method10);
			ilg.Call(method9);
			ilg.Ldc(boolVar: false);
			ilg.AndIf(Cmp.EqualTo);
		}
		if (anyAttribute != null)
		{
			LocalBuilder localBuilder = ilg.DeclareOrGetLocal(typeof(XmlAttribute), "attr");
			MethodInfo method11 = typeof(XmlSerializationReader).GetMethod("get_Document", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			MethodInfo method12 = typeof(XmlDocument).GetMethod("ReadNode", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(XmlReader) }, null);
			ilg.Ldarg(0);
			ilg.Call(method11);
			ilg.Ldarg(0);
			ilg.Call(method);
			ilg.Call(method12);
			ilg.ConvertValue(method12.ReturnType, localBuilder.LocalType);
			ilg.Stloc(localBuilder);
			MethodInfo method13 = typeof(XmlSerializationReader).GetMethod("ParseWsdlArrayType", CodeGenerator.InstanceBindingFlags, null, new Type[1] { localBuilder.LocalType }, null);
			ilg.Ldarg(0);
			ilg.Ldloc(localBuilder);
			ilg.Call(method13);
			WriteAttribute(anyAttribute);
		}
		else
		{
			List<Type> list = new List<Type>();
			ilg.Ldarg(0);
			list.Add(typeof(object));
			ilg.Ldloc(firstParam);
			ilg.ConvertValue(firstParam.LocalType, typeof(object));
			if (arrayList.Count > 0)
			{
				string text = "";
				for (int j = 0; j < arrayList.Count; j++)
				{
					AttributeAccessor attributeAccessor = (AttributeAccessor)arrayList[j];
					if (j > 0)
					{
						text += ", ";
					}
					text += (attributeAccessor.IsSpecialXmlNamespace ? "http://www.w3.org/XML/1998/namespace" : (((attributeAccessor.Form == XmlSchemaForm.Qualified) ? attributeAccessor.Namespace : "") + ":" + attributeAccessor.Name));
				}
				list.Add(typeof(string));
				ilg.Ldstr(text);
			}
			MethodInfo method14 = typeof(XmlSerializationReader).GetMethod(elseCall, CodeGenerator.InstanceBindingFlags, null, list.ToArray(), null);
			ilg.Call(method14);
		}
		ilg.EndIf();
		ilg.WhileBeginCondition();
		ilg.Ldarg(0);
		ilg.Call(method);
		ilg.Call(method2);
		ilg.WhileEndCondition();
		ilg.WhileEnd();
	}

	private void WriteAttribute(Member member)
	{
		AttributeAccessor attribute = member.Mapping.Attribute;
		if (attribute.Mapping is SpecialMapping)
		{
			SpecialMapping specialMapping = (SpecialMapping)attribute.Mapping;
			if (specialMapping.TypeDesc.Kind == TypeKind.Attribute)
			{
				WriteSourceBegin(member.ArraySource);
				ilg.Ldloc("attr");
				WriteSourceEnd(member.ArraySource, member.Mapping.TypeDesc.IsArrayLike ? member.Mapping.TypeDesc.ArrayElementTypeDesc.Type : member.Mapping.TypeDesc.Type);
			}
			else
			{
				if (!specialMapping.TypeDesc.CanBeAttributeValue)
				{
					throw new InvalidOperationException(Res.GetString("Internal error."));
				}
				LocalBuilder local = ilg.GetLocal("attr");
				ilg.Ldloc(local);
				if (local.LocalType == typeof(XmlAttribute))
				{
					ilg.Load(null);
					ilg.Cne();
				}
				else
				{
					ilg.IsInst(typeof(XmlAttribute));
				}
				ilg.If();
				WriteSourceBegin(member.ArraySource);
				ilg.Ldloc(local);
				ilg.ConvertValue(local.LocalType, typeof(XmlAttribute));
				WriteSourceEnd(member.ArraySource, member.Mapping.TypeDesc.IsArrayLike ? member.Mapping.TypeDesc.ArrayElementTypeDesc.Type : member.Mapping.TypeDesc.Type);
				ilg.EndIf();
			}
		}
		else if (attribute.IsList)
		{
			LocalBuilder localBuilder = ilg.DeclareOrGetLocal(typeof(string), "listValues");
			LocalBuilder localBuilder2 = ilg.DeclareOrGetLocal(typeof(string[]), "vals");
			MethodInfo method = typeof(string).GetMethod("Split", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(char[]) }, null);
			MethodInfo method2 = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			MethodInfo method3 = typeof(XmlReader).GetMethod("get_Value", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldarg(0);
			ilg.Call(method2);
			ilg.Call(method3);
			ilg.Stloc(localBuilder);
			ilg.Ldloc(localBuilder);
			ilg.Load(null);
			ilg.Call(method);
			ilg.Stloc(localBuilder2);
			LocalBuilder local2 = ilg.DeclareOrGetLocal(typeof(int), "i");
			ilg.For(local2, 0, localBuilder2);
			string arraySource = GetArraySource(member.Mapping.TypeDesc, member.ArrayName);
			WriteSourceBegin(arraySource);
			WritePrimitive(attribute.Mapping, "vals[i]");
			WriteSourceEnd(arraySource, member.Mapping.TypeDesc.ArrayElementTypeDesc.Type);
			ilg.EndFor();
		}
		else
		{
			WriteSourceBegin(member.ArraySource);
			WritePrimitive(attribute.Mapping, attribute.IsList ? "vals[i]" : "Reader.Value");
			WriteSourceEnd(member.ArraySource, member.Mapping.TypeDesc.IsArrayLike ? member.Mapping.TypeDesc.ArrayElementTypeDesc.Type : member.Mapping.TypeDesc.Type);
		}
		if (member.Mapping.CheckSpecified == SpecifiedAccessor.ReadWrite && member.CheckSpecifiedSource != null && member.CheckSpecifiedSource.Length > 0)
		{
			ILGenSet(member.CheckSpecifiedSource, true);
		}
		if (member.ParamsReadSource != null)
		{
			ILGenParamsReadSource(member.ParamsReadSource, value: true);
		}
	}

	private void WriteMemberBegin(Member[] members)
	{
		foreach (Member member in members)
		{
			if (!member.IsArrayLike)
			{
				continue;
			}
			string arrayName = member.ArrayName;
			string name = "c" + arrayName;
			TypeDesc typeDesc = member.Mapping.TypeDesc;
			if (member.Mapping.TypeDesc.IsArray)
			{
				WriteArrayLocalDecl(typeDesc.CSharpName, arrayName, "null", typeDesc);
				ilg.Ldc(0);
				ilg.Stloc(typeof(int), name);
				if (member.Mapping.ChoiceIdentifier != null)
				{
					WriteArrayLocalDecl(member.Mapping.ChoiceIdentifier.Mapping.TypeDesc.CSharpName + "[]", member.ChoiceArrayName, "null", member.Mapping.ChoiceIdentifier.Mapping.TypeDesc);
					ilg.Ldc(0);
					ilg.Stloc(typeof(int), "c" + member.ChoiceArrayName);
				}
				continue;
			}
			if (member.Source[member.Source.Length - 1] == '(' || member.Source[member.Source.Length - 1] == '{')
			{
				WriteCreateInstance(arrayName, typeDesc.CannotNew, typeDesc.Type);
				WriteSourceBegin(member.Source);
				ilg.Ldloc(ilg.GetLocal(arrayName));
				WriteSourceEnd(member.Source, typeDesc.Type);
				continue;
			}
			if (member.IsList && !member.Mapping.ReadOnly && member.Mapping.TypeDesc.IsNullable)
			{
				ILGenLoad(member.Source, typeof(object));
				ilg.Load(null);
				ilg.If(Cmp.EqualTo);
				if (!member.Mapping.TypeDesc.HasDefaultConstructor)
				{
					MethodInfo method = typeof(XmlSerializationReader).GetMethod("CreateReadOnlyCollectionException", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(string) }, null);
					ilg.Ldarg(0);
					ilg.Ldstr(member.Mapping.TypeDesc.CSharpName);
					ilg.Call(method);
					ilg.Throw();
				}
				else
				{
					WriteSourceBegin(member.Source);
					base.RaCodeGen.ILGenForCreateInstance(ilg, member.Mapping.TypeDesc.Type, typeDesc.CannotNew, cast: true);
					WriteSourceEnd(member.Source, member.Mapping.TypeDesc.Type);
				}
				ilg.EndIf();
			}
			WriteLocalDecl(arrayName, new SourceInfo(member.Source, member.Source, member.Mapping.MemberInfo, member.Mapping.TypeDesc.Type, ilg));
		}
	}

	private string ExpectedElements(Member[] members)
	{
		if (IsSequence(members))
		{
			return "null";
		}
		string text = string.Empty;
		bool flag = true;
		foreach (Member member in members)
		{
			if (member.Mapping.Xmlns != null || member.Mapping.Ignore || member.Mapping.IsText || member.Mapping.IsAttribute)
			{
				continue;
			}
			ElementAccessor[] elements = member.Mapping.Elements;
			foreach (ElementAccessor elementAccessor in elements)
			{
				string text2 = ((elementAccessor.Form == XmlSchemaForm.Qualified) ? elementAccessor.Namespace : "");
				if (!elementAccessor.Any || (elementAccessor.Name != null && elementAccessor.Name.Length != 0))
				{
					if (!flag)
					{
						text += ", ";
					}
					text = text + text2 + ":" + elementAccessor.Name;
					flag = false;
				}
			}
		}
		return ReflectionAwareILGen.GetQuotedCSharpString(null, text);
	}

	private void WriteMemberElements(Member[] members, string elementElseString, string elseString, Member anyElement, Member anyText)
	{
		if (anyText != null)
		{
			ilg.Load(null);
			ilg.Stloc(typeof(string), "tmp");
		}
		MethodInfo method = typeof(XmlReader).GetMethod("get_NodeType", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		MethodInfo method2 = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		int intVar = 1;
		ilg.Ldarg(0);
		ilg.Call(method2);
		ilg.Call(method);
		ilg.Ldc(intVar);
		ilg.If(Cmp.EqualTo);
		WriteMemberElementsIf(members, anyElement, elementElseString);
		if (anyText != null)
		{
			WriteMemberText(anyText, elseString);
		}
		ilg.Else();
		ILGenElseString(elseString);
		ilg.EndIf();
	}

	private void WriteMemberText(Member anyText, string elseString)
	{
		ilg.InitElseIf();
		Label label = ilg.DefineLabel();
		Label label2 = ilg.DefineLabel();
		MethodInfo method = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		MethodInfo method2 = typeof(XmlReader).GetMethod("get_NodeType", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg.Ldarg(0);
		ilg.Call(method);
		ilg.Call(method2);
		ilg.Ldc(XmlNodeType.Text);
		ilg.Ceq();
		ilg.Brtrue(label);
		ilg.Ldarg(0);
		ilg.Call(method);
		ilg.Call(method2);
		ilg.Ldc(XmlNodeType.CDATA);
		ilg.Ceq();
		ilg.Brtrue(label);
		ilg.Ldarg(0);
		ilg.Call(method);
		ilg.Call(method2);
		ilg.Ldc(XmlNodeType.Whitespace);
		ilg.Ceq();
		ilg.Brtrue(label);
		ilg.Ldarg(0);
		ilg.Call(method);
		ilg.Call(method2);
		ilg.Ldc(XmlNodeType.SignificantWhitespace);
		ilg.Ceq();
		ilg.Br(label2);
		ilg.MarkLabel(label);
		ilg.Ldc(boolVar: true);
		ilg.MarkLabel(label2);
		ilg.AndIf();
		if (anyText != null)
		{
			WriteText(anyText);
		}
	}

	private void WriteText(Member member)
	{
		TextAccessor text = member.Mapping.Text;
		if (text.Mapping is SpecialMapping)
		{
			SpecialMapping specialMapping = (SpecialMapping)text.Mapping;
			WriteSourceBeginTyped(member.ArraySource, specialMapping.TypeDesc);
			TypeKind kind = specialMapping.TypeDesc.Kind;
			if (kind == TypeKind.Node)
			{
				MethodInfo method = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				MethodInfo method2 = typeof(XmlReader).GetMethod("ReadString", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				MethodInfo method3 = typeof(XmlSerializationReader).GetMethod("get_Document", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				MethodInfo method4 = typeof(XmlDocument).GetMethod("CreateTextNode", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(string) }, null);
				ilg.Ldarg(0);
				ilg.Call(method3);
				ilg.Ldarg(0);
				ilg.Call(method);
				ilg.Call(method2);
				ilg.Call(method4);
				WriteSourceEnd(member.ArraySource, specialMapping.TypeDesc.Type);
				return;
			}
			throw new InvalidOperationException(Res.GetString("Internal error."));
		}
		if (member.IsArrayLike)
		{
			WriteSourceBegin(member.ArraySource);
			if (text.Mapping.TypeDesc.CollapseWhitespace)
			{
				ilg.Ldarg(0);
			}
			MethodInfo method5 = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			MethodInfo method6 = typeof(XmlReader).GetMethod("ReadString", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldarg(0);
			ilg.Call(method5);
			ilg.Call(method6);
			if (text.Mapping.TypeDesc.CollapseWhitespace)
			{
				MethodInfo method7 = typeof(XmlSerializationReader).GetMethod("CollapseWhitespace", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(string) }, null);
				ilg.Call(method7);
			}
		}
		else if (text.Mapping.TypeDesc == base.StringTypeDesc || text.Mapping.TypeDesc.FormatterName == "String")
		{
			LocalBuilder local = ilg.GetLocal("tmp");
			MethodInfo method8 = typeof(XmlSerializationReader).GetMethod("ReadString", CodeGenerator.InstanceBindingFlags, null, new Type[2]
			{
				typeof(string),
				typeof(bool)
			}, null);
			ilg.Ldarg(0);
			ilg.Ldloc(local);
			ilg.Ldc(text.Mapping.TypeDesc.CollapseWhitespace);
			ilg.Call(method8);
			ilg.Stloc(local);
			WriteSourceBegin(member.ArraySource);
			ilg.Ldloc(local);
		}
		else
		{
			WriteSourceBegin(member.ArraySource);
			WritePrimitive(text.Mapping, "Reader.ReadString()");
		}
		WriteSourceEnd(member.ArraySource, text.Mapping.TypeDesc.Type);
	}

	private void WriteMemberElementsElse(Member anyElement, string elementElseString)
	{
		if (anyElement != null)
		{
			ElementAccessor[] elements = anyElement.Mapping.Elements;
			for (int i = 0; i < elements.Length; i++)
			{
				ElementAccessor elementAccessor = elements[i];
				if (elementAccessor.Any && elementAccessor.Name.Length == 0)
				{
					WriteElement(anyElement.ArraySource, anyElement.ArrayName, anyElement.ChoiceArraySource, elementAccessor, anyElement.Mapping.ChoiceIdentifier, (anyElement.Mapping.CheckSpecified == SpecifiedAccessor.ReadWrite) ? anyElement.CheckSpecifiedSource : null, checkForNull: false, readOnly: false, -1, i);
					break;
				}
			}
		}
		else
		{
			ILGenElementElseString(elementElseString);
		}
	}

	private bool IsSequence(Member[] members)
	{
		for (int i = 0; i < members.Length; i++)
		{
			if (members[i].Mapping.IsParticle && members[i].Mapping.IsSequence)
			{
				return true;
			}
		}
		return false;
	}

	private void WriteMemberElementsIf(Member[] members, Member anyElement, string elementElseString)
	{
		int num = 0;
		bool flag = IsSequence(members);
		int num2 = 0;
		foreach (Member member in members)
		{
			if (member.Mapping.Xmlns != null || member.Mapping.Ignore || (flag && (member.Mapping.IsText || member.Mapping.IsAttribute)))
			{
				continue;
			}
			bool flag2 = true;
			ChoiceIdentifierAccessor choiceIdentifier = member.Mapping.ChoiceIdentifier;
			ElementAccessor[] elements = member.Mapping.Elements;
			for (int j = 0; j < elements.Length; j++)
			{
				ElementAccessor elementAccessor = elements[j];
				string ns = ((elementAccessor.Form == XmlSchemaForm.Qualified) ? elementAccessor.Namespace : "");
				if (!flag && elementAccessor.Any && (elementAccessor.Name == null || elementAccessor.Name.Length == 0))
				{
					continue;
				}
				if (!flag2 || (!flag && num > 0))
				{
					ilg.InitElseIf();
				}
				else if (flag)
				{
					if (num2 > 0)
					{
						ilg.InitElseIf();
					}
					else
					{
						ilg.InitIf();
					}
					ilg.Ldloc("state");
					ilg.Ldc(num2);
					ilg.AndIf(Cmp.EqualTo);
					ilg.InitIf();
				}
				else
				{
					ilg.InitIf();
				}
				num++;
				flag2 = false;
				if (member.ParamsReadSource != null)
				{
					ILGenParamsReadSource(member.ParamsReadSource);
					ilg.Ldc(boolVar: false);
					ilg.AndIf(Cmp.EqualTo);
				}
				Label label = ilg.DefineLabel();
				Label label2 = ilg.DefineLabel();
				if (member.Mapping.IsReturnValue)
				{
					MethodInfo method = typeof(XmlSerializationReader).GetMethod("get_IsReturnValue", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
					ilg.Ldarg(0);
					ilg.Call(method);
					ilg.Brtrue(label);
				}
				if (flag && elementAccessor.Any && elementAccessor.AnyNamespaces == null)
				{
					ilg.Ldc(boolVar: true);
				}
				else
				{
					WriteXmlNodeEqual("Reader", elementAccessor.Name, ns, doAndIf: false);
				}
				if (member.Mapping.IsReturnValue)
				{
					ilg.Br_S(label2);
					ilg.MarkLabel(label);
					ilg.Ldc(boolVar: true);
					ilg.MarkLabel(label2);
				}
				ilg.AndIf();
				WriteElement(member.ArraySource, member.ArrayName, member.ChoiceArraySource, elementAccessor, choiceIdentifier, (member.Mapping.CheckSpecified == SpecifiedAccessor.ReadWrite) ? member.CheckSpecifiedSource : null, member.IsList && member.Mapping.TypeDesc.IsNullable, member.Mapping.ReadOnly, member.FixupIndex, j);
				if (member.Mapping.IsReturnValue)
				{
					MethodInfo method2 = typeof(XmlSerializationReader).GetMethod("set_IsReturnValue", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(bool) }, null);
					ilg.Ldarg(0);
					ilg.Ldc(boolVar: false);
					ilg.Call(method2);
				}
				if (member.ParamsReadSource != null)
				{
					ILGenParamsReadSource(member.ParamsReadSource, value: true);
				}
			}
			if (flag)
			{
				if (member.IsArrayLike)
				{
					ilg.Else();
				}
				else
				{
					ilg.EndIf();
				}
				num2++;
				ilg.Ldc(num2);
				ilg.Stloc(ilg.GetLocal("state"));
				if (member.IsArrayLike)
				{
					ilg.EndIf();
				}
			}
		}
		if (num > 0)
		{
			ilg.Else();
		}
		WriteMemberElementsElse(anyElement, elementElseString);
		if (num > 0)
		{
			ilg.EndIf();
		}
	}

	private string GetArraySource(TypeDesc typeDesc, string arrayName)
	{
		return GetArraySource(typeDesc, arrayName, multiRef: false);
	}

	private string GetArraySource(TypeDesc typeDesc, string arrayName, bool multiRef)
	{
		string text = "c" + arrayName;
		string text2 = "";
		if (multiRef)
		{
			text2 = "soap = (System.Object[])EnsureArrayIndex(soap, " + text + "+2, typeof(System.Object)); ";
		}
		if (typeDesc.IsArray)
		{
			string cSharpName = typeDesc.ArrayElementTypeDesc.CSharpName;
			string text3 = "(" + cSharpName + "[])";
			text2 = text2 + arrayName + " = " + text3 + "EnsureArrayIndex(" + arrayName + ", " + text + ", " + base.RaCodeGen.GetStringForTypeof(cSharpName) + ");";
			string stringForArrayMember = base.RaCodeGen.GetStringForArrayMember(arrayName, text + "++", typeDesc);
			if (multiRef)
			{
				text2 = text2 + " soap[1] = " + arrayName + ";";
				text2 = text2 + " if (ReadReference(out soap[" + text + "+2])) " + stringForArrayMember + " = null; else ";
			}
			return text2 + stringForArrayMember;
		}
		return base.RaCodeGen.GetStringForMethod(arrayName, typeDesc.CSharpName, "Add");
	}

	private void WriteMemberEnd(Member[] members)
	{
		WriteMemberEnd(members, soapRefs: false);
	}

	private void WriteMemberEnd(Member[] members, bool soapRefs)
	{
		foreach (Member member in members)
		{
			if (!member.IsArrayLike)
			{
				continue;
			}
			TypeDesc typeDesc = member.Mapping.TypeDesc;
			if (typeDesc.IsArray)
			{
				WriteSourceBegin(member.Source);
				string arrayName = member.ArrayName;
				string name = "c" + arrayName;
				MethodInfo method = typeof(XmlSerializationReader).GetMethod("ShrinkArray", CodeGenerator.InstanceBindingFlags, null, new Type[4]
				{
					typeof(Array),
					typeof(int),
					typeof(Type),
					typeof(bool)
				}, null);
				ilg.Ldarg(0);
				ilg.Ldloc(ilg.GetLocal(arrayName));
				ilg.Ldloc(ilg.GetLocal(name));
				ilg.Ldc(typeDesc.ArrayElementTypeDesc.Type);
				ilg.Ldc(member.IsNullable);
				ilg.Call(method);
				ilg.ConvertValue(method.ReturnType, typeDesc.Type);
				WriteSourceEnd(member.Source, typeDesc.Type);
				if (member.Mapping.ChoiceIdentifier != null)
				{
					WriteSourceBegin(member.ChoiceSource);
					arrayName = member.ChoiceArrayName;
					name = "c" + arrayName;
					ilg.Ldarg(0);
					ilg.Ldloc(ilg.GetLocal(arrayName));
					ilg.Ldloc(ilg.GetLocal(name));
					ilg.Ldc(member.Mapping.ChoiceIdentifier.Mapping.TypeDesc.Type);
					ilg.Ldc(member.IsNullable);
					ilg.Call(method);
					ilg.ConvertValue(method.ReturnType, member.Mapping.ChoiceIdentifier.Mapping.TypeDesc.Type.MakeArrayType());
					WriteSourceEnd(member.ChoiceSource, member.Mapping.ChoiceIdentifier.Mapping.TypeDesc.Type.MakeArrayType());
				}
			}
			else if (typeDesc.IsValueType)
			{
				LocalBuilder local = ilg.GetLocal(member.ArrayName);
				WriteSourceBegin(member.Source);
				ilg.Ldloc(local);
				WriteSourceEnd(member.Source, local.LocalType);
			}
		}
	}

	private void WriteSourceBeginTyped(string source, TypeDesc typeDesc)
	{
		WriteSourceBegin(source);
	}

	private void WriteSourceBegin(string source)
	{
		if (ilg.TryGetVariable(source, out var variable))
		{
			if (CodeGenerator.IsNullableGenericType(ilg.GetVariableType(variable)))
			{
				ilg.LoadAddress(variable);
			}
			return;
		}
		if (source.StartsWith("o.@", StringComparison.Ordinal))
		{
			ilg.LdlocAddress(ilg.GetLocal("o"));
			return;
		}
		Match match = XmlSerializationILGen.NewRegex("(?<locA1>[^ ]+) = .+EnsureArrayIndex[(](?<locA2>[^,]+), (?<locI1>[^,]+),[^;]+;(?<locA3>[^[]+)[[](?<locI2>[^+]+)[+][+][]]").Match(source);
		if (match.Success)
		{
			LocalBuilder local = ilg.GetLocal(match.Groups["locA1"].Value);
			LocalBuilder local2 = ilg.GetLocal(match.Groups["locI1"].Value);
			Type elementType = local.LocalType.GetElementType();
			MethodInfo method = typeof(XmlSerializationReader).GetMethod("EnsureArrayIndex", CodeGenerator.InstanceBindingFlags, null, new Type[3]
			{
				typeof(Array),
				typeof(int),
				typeof(Type)
			}, null);
			ilg.Ldarg(0);
			ilg.Ldloc(local);
			ilg.Ldloc(local2);
			ilg.Ldc(elementType);
			ilg.Call(method);
			ilg.Castclass(local.LocalType);
			ilg.Stloc(local);
			ilg.Ldloc(local);
			ilg.Ldloc(local2);
			ilg.Dup();
			ilg.Ldc(1);
			ilg.Add();
			ilg.Stloc(local2);
			if (CodeGenerator.IsNullableGenericType(elementType) || elementType.IsValueType)
			{
				ilg.Ldelema(elementType);
			}
		}
		else if (source.EndsWith(".Add(", StringComparison.Ordinal))
		{
			int length = source.LastIndexOf(".Add(", StringComparison.Ordinal);
			LocalBuilder local3 = ilg.GetLocal(source.Substring(0, length));
			ilg.LdlocAddress(local3);
		}
		else
		{
			match = XmlSerializationILGen.NewRegex("(?<a>[^[]+)[[](?<ia>.+)[]]").Match(source);
			if (!match.Success)
			{
				throw CodeGenerator.NotSupported("Unexpected: " + source);
			}
			ilg.Load(ilg.GetVariable(match.Groups["a"].Value));
			ilg.Load(ilg.GetVariable(match.Groups["ia"].Value));
		}
	}

	private void WriteSourceEnd(string source, Type elementType)
	{
		WriteSourceEnd(source, elementType, elementType);
	}

	private void WriteSourceEnd(string source, Type elementType, Type stackType)
	{
		if (ilg.TryGetVariable(source, out var variable))
		{
			Type variableType = ilg.GetVariableType(variable);
			if (CodeGenerator.IsNullableGenericType(variableType))
			{
				ilg.Call(variableType.GetConstructor(variableType.GetGenericArguments()));
				return;
			}
			ilg.ConvertValue(stackType, elementType);
			ilg.ConvertValue(elementType, variableType);
			ilg.Stloc((LocalBuilder)variable);
			return;
		}
		if (source.StartsWith("o.@", StringComparison.Ordinal))
		{
			MemberInfo memberInfo = memberInfos[source.Substring(3)];
			ilg.ConvertValue(stackType, (memberInfo.MemberType == MemberTypes.Field) ? ((FieldInfo)memberInfo).FieldType : ((PropertyInfo)memberInfo).PropertyType);
			ilg.StoreMember(memberInfo);
			return;
		}
		Match match = XmlSerializationILGen.NewRegex("(?<locA1>[^ ]+) = .+EnsureArrayIndex[(](?<locA2>[^,]+), (?<locI1>[^,]+),[^;]+;(?<locA3>[^[]+)[[](?<locI2>[^+]+)[+][+][]]").Match(source);
		if (match.Success)
		{
			object variable2 = ilg.GetVariable(match.Groups["locA1"].Value);
			Type elementType2 = ilg.GetVariableType(variable2).GetElementType();
			ilg.ConvertValue(elementType, elementType2);
			if (CodeGenerator.IsNullableGenericType(elementType2) || elementType2.IsValueType)
			{
				ilg.Stobj(elementType2);
			}
			else
			{
				ilg.Stelem(elementType2);
			}
		}
		else if (source.EndsWith(".Add(", StringComparison.Ordinal))
		{
			int length = source.LastIndexOf(".Add(", StringComparison.Ordinal);
			MethodInfo method = ilg.GetLocal(source.Substring(0, length)).LocalType.GetMethod("Add", CodeGenerator.InstanceBindingFlags, null, new Type[1] { elementType }, null);
			Type parameterType = method.GetParameters()[0].ParameterType;
			ilg.ConvertValue(stackType, parameterType);
			ilg.Call(method);
			if (method.ReturnType != typeof(void))
			{
				ilg.Pop();
			}
		}
		else
		{
			match = XmlSerializationILGen.NewRegex("(?<a>[^[]+)[[](?<ia>.+)[]]").Match(source);
			if (!match.Success)
			{
				throw CodeGenerator.NotSupported("Unexpected: " + source);
			}
			Type elementType3 = ilg.GetVariableType(ilg.GetVariable(match.Groups["a"].Value)).GetElementType();
			ilg.ConvertValue(stackType, elementType3);
			ilg.Stelem(elementType3);
		}
	}

	private void WriteArray(string source, string arrayName, ArrayMapping arrayMapping, bool readOnly, bool isNullable, int fixupIndex, int elementIndex)
	{
		MethodInfo method = typeof(XmlSerializationReader).GetMethod("ReadNull", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg.Ldarg(0);
		ilg.Call(method);
		ilg.IfNot();
		MemberMapping memberMapping = new MemberMapping();
		memberMapping.Elements = arrayMapping.Elements;
		memberMapping.TypeDesc = arrayMapping.TypeDesc;
		memberMapping.ReadOnly = readOnly;
		if (source.StartsWith("o.@", StringComparison.Ordinal))
		{
			memberMapping.MemberInfo = memberInfos[source.Substring(3)];
		}
		Member member = new Member(this, source, arrayName, elementIndex, memberMapping, multiRef: false);
		member.IsNullable = false;
		Member[] members = new Member[1] { member };
		WriteMemberBegin(members);
		Label label = ilg.DefineLabel();
		Label label2 = ilg.DefineLabel();
		if (readOnly)
		{
			ilg.Load(ilg.GetVariable(member.ArrayName));
			ilg.Load(null);
			ilg.Beq(label);
		}
		MethodInfo method2 = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		MethodInfo method3 = typeof(XmlReader).GetMethod("get_IsEmptyElement", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg.Ldarg(0);
		ilg.Call(method2);
		ilg.Call(method3);
		if (readOnly)
		{
			ilg.Br_S(label2);
			ilg.MarkLabel(label);
			ilg.Ldc(boolVar: true);
			ilg.MarkLabel(label2);
		}
		ilg.If();
		MethodInfo method4 = typeof(XmlReader).GetMethod("Skip", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg.Ldarg(0);
		ilg.Call(method2);
		ilg.Call(method4);
		ilg.Else();
		MethodInfo method5 = typeof(XmlReader).GetMethod("ReadStartElement", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg.Ldarg(0);
		ilg.Call(method2);
		ilg.Call(method5);
		int loopIndex = WriteWhileNotLoopStart();
		string text = "UnknownNode(null, " + ExpectedElements(members) + ");";
		WriteMemberElements(members, text, text, null, null);
		MethodInfo method6 = typeof(XmlReader).GetMethod("MoveToContent", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg.Ldarg(0);
		ilg.Call(method2);
		ilg.Call(method6);
		ilg.Pop();
		WriteWhileLoopEnd(loopIndex);
		MethodInfo method7 = typeof(XmlSerializationReader).GetMethod("ReadEndElement", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg.Ldarg(0);
		ilg.Call(method7);
		ilg.EndIf();
		WriteMemberEnd(members, soapRefs: false);
		if (isNullable)
		{
			ilg.Else();
			member.IsNullable = true;
			WriteMemberBegin(members);
			WriteMemberEnd(members);
		}
		ilg.EndIf();
	}

	private void WriteElement(string source, string arrayName, string choiceSource, ElementAccessor element, ChoiceIdentifierAccessor choice, string checkSpecified, bool checkForNull, bool readOnly, int fixupIndex, int elementIndex)
	{
		if (checkSpecified != null && checkSpecified.Length > 0)
		{
			ILGenSet(checkSpecified, true);
		}
		if (element.Mapping is ArrayMapping)
		{
			WriteArray(source, arrayName, (ArrayMapping)element.Mapping, readOnly, element.IsNullable, fixupIndex, elementIndex);
		}
		else if (element.Mapping is NullableMapping)
		{
			string methodName = ReferenceMapping(element.Mapping);
			WriteSourceBegin(source);
			ilg.Ldarg(0);
			ilg.Ldc(boolVar: true);
			MethodBuilder methodInfo = EnsureMethodBuilder(typeBuilder, methodName, CodeGenerator.PrivateMethodAttributes, element.Mapping.TypeDesc.Type, new Type[1] { typeof(bool) });
			ilg.Call(methodInfo);
			WriteSourceEnd(source, element.Mapping.TypeDesc.Type);
		}
		else if (element.Mapping is PrimitiveMapping)
		{
			bool flag = false;
			if (element.IsNullable)
			{
				MethodInfo method = typeof(XmlSerializationReader).GetMethod("ReadNull", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				ilg.Ldarg(0);
				ilg.Call(method);
				ilg.If();
				WriteSourceBegin(source);
				if (element.Mapping.TypeDesc.IsValueType)
				{
					throw CodeGenerator.NotSupported("No such condition.  PrimitiveMapping && IsNullable = String, XmlQualifiedName and never IsValueType");
				}
				ilg.Load(null);
				WriteSourceEnd(source, element.Mapping.TypeDesc.Type);
				ilg.Else();
				flag = true;
			}
			if (element.Default != null && element.Default != DBNull.Value && element.Mapping.TypeDesc.IsValueType)
			{
				MethodInfo method2 = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				MethodInfo method3 = typeof(XmlReader).GetMethod("get_IsEmptyElement", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				ilg.Ldarg(0);
				ilg.Call(method2);
				ilg.Call(method3);
				ilg.If();
				MethodInfo method4 = typeof(XmlReader).GetMethod("Skip", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				ilg.Ldarg(0);
				ilg.Call(method2);
				ilg.Call(method4);
				ilg.Else();
				flag = true;
			}
			if (System.LocalAppContextSwitches.EnableTimeSpanSerialization && element.Mapping.TypeDesc.Type == typeof(TimeSpan))
			{
				MethodInfo method5 = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				MethodInfo method6 = typeof(XmlReader).GetMethod("get_IsEmptyElement", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				ilg.Ldarg(0);
				ilg.Call(method5);
				ilg.Call(method6);
				ilg.If();
				WriteSourceBegin(source);
				MethodInfo method7 = typeof(XmlReader).GetMethod("Skip", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				ilg.Ldarg(0);
				ilg.Call(method5);
				ilg.Call(method7);
				ConstructorInfo constructor = typeof(TimeSpan).GetConstructor(CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(long) }, null);
				ilg.Ldc(default(TimeSpan).Ticks);
				ilg.New(constructor);
				WriteSourceEnd(source, element.Mapping.TypeDesc.Type);
				ilg.Else();
				WriteSourceBegin(source);
				WritePrimitive(element.Mapping, "Reader.ReadElementString()");
				WriteSourceEnd(source, element.Mapping.TypeDesc.Type);
				ilg.EndIf();
			}
			else
			{
				WriteSourceBegin(source);
				if (element.Mapping.TypeDesc == base.QnameTypeDesc)
				{
					MethodInfo method8 = typeof(XmlSerializationReader).GetMethod("ReadElementQualifiedName", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
					ilg.Ldarg(0);
					ilg.Call(method8);
				}
				else
				{
					string formatterName = element.Mapping.TypeDesc.FormatterName;
					WritePrimitive(source: (!(formatterName == "ByteArrayBase64") && !(formatterName == "ByteArrayHex")) ? "Reader.ReadElementString()" : "false", mapping: element.Mapping);
				}
				WriteSourceEnd(source, element.Mapping.TypeDesc.Type);
			}
			if (flag)
			{
				ilg.EndIf();
			}
		}
		else if (element.Mapping is StructMapping)
		{
			TypeMapping mapping = element.Mapping;
			string methodName2 = ReferenceMapping(mapping);
			if (checkForNull)
			{
				MethodInfo method9 = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				MethodInfo method10 = typeof(XmlReader).GetMethod("Skip", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
				ilg.Ldloc(arrayName);
				ilg.Load(null);
				ilg.If(Cmp.EqualTo);
				ilg.Ldarg(0);
				ilg.Call(method9);
				ilg.Call(method10);
				ilg.Else();
			}
			WriteSourceBegin(source);
			List<Type> list = new List<Type>();
			ilg.Ldarg(0);
			if (mapping.TypeDesc.IsNullable)
			{
				ilg.Load(element.IsNullable);
				list.Add(typeof(bool));
			}
			ilg.Ldc(boolVar: true);
			list.Add(typeof(bool));
			MethodBuilder methodInfo2 = EnsureMethodBuilder(typeBuilder, methodName2, CodeGenerator.PrivateMethodAttributes, mapping.TypeDesc.Type, list.ToArray());
			ilg.Call(methodInfo2);
			WriteSourceEnd(source, mapping.TypeDesc.Type);
			if (checkForNull)
			{
				ilg.EndIf();
			}
		}
		else
		{
			if (!(element.Mapping is SpecialMapping))
			{
				throw new InvalidOperationException(Res.GetString("Internal error."));
			}
			SpecialMapping specialMapping = (SpecialMapping)element.Mapping;
			switch (specialMapping.TypeDesc.Kind)
			{
			case TypeKind.Node:
			{
				bool flag3 = specialMapping.TypeDesc.FullName == typeof(XmlDocument).FullName;
				WriteSourceBeginTyped(source, specialMapping.TypeDesc);
				MethodInfo method13 = typeof(XmlSerializationReader).GetMethod(flag3 ? "ReadXmlDocument" : "ReadXmlNode", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(bool) }, null);
				ilg.Ldarg(0);
				ilg.Ldc(!element.Any);
				ilg.Call(method13);
				if (specialMapping.TypeDesc != null)
				{
					ilg.Castclass(specialMapping.TypeDesc.Type);
				}
				WriteSourceEnd(source, specialMapping.TypeDesc.Type);
				break;
			}
			case TypeKind.Serializable:
			{
				SerializableMapping serializableMapping = (SerializableMapping)element.Mapping;
				if (serializableMapping.DerivedMappings != null)
				{
					MethodInfo method11 = typeof(XmlSerializationReader).GetMethod("GetXsiType", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
					Label label = ilg.DefineLabel();
					Label label2 = ilg.DefineLabel();
					LocalBuilder localBuilder = ilg.DeclareOrGetLocal(typeof(XmlQualifiedName), "tser");
					ilg.Ldarg(0);
					ilg.Call(method11);
					ilg.Stloc(localBuilder);
					ilg.Ldloc(localBuilder);
					ilg.Load(null);
					ilg.Ceq();
					ilg.Brtrue(label);
					WriteQNameEqual("tser", serializableMapping.XsiType.Name, serializableMapping.XsiType.Namespace);
					ilg.Br_S(label2);
					ilg.MarkLabel(label);
					ilg.Ldc(boolVar: true);
					ilg.MarkLabel(label2);
					ilg.If();
				}
				WriteSourceBeginTyped(source, serializableMapping.TypeDesc);
				bool flag2 = !element.Any && XmlSerializationILGen.IsWildcard(serializableMapping);
				MethodInfo method12 = typeof(XmlSerializationReader).GetMethod("ReadSerializable", CodeGenerator.InstanceBindingFlags, null, (!flag2) ? new Type[1] { typeof(IXmlSerializable) } : new Type[2]
				{
					typeof(IXmlSerializable),
					typeof(bool)
				}, null);
				ilg.Ldarg(0);
				base.RaCodeGen.ILGenForCreateInstance(ilg, serializableMapping.TypeDesc.Type, serializableMapping.TypeDesc.CannotNew, cast: false);
				if (serializableMapping.TypeDesc.CannotNew)
				{
					ilg.ConvertValue(typeof(object), typeof(IXmlSerializable));
				}
				if (flag2)
				{
					ilg.Ldc(boolVar: true);
				}
				ilg.Call(method12);
				if (serializableMapping.TypeDesc != null)
				{
					ilg.ConvertValue(typeof(IXmlSerializable), serializableMapping.TypeDesc.Type);
				}
				WriteSourceEnd(source, serializableMapping.TypeDesc.Type);
				if (serializableMapping.DerivedMappings != null)
				{
					WriteDerivedSerializable(serializableMapping, serializableMapping, source, flag2);
					WriteUnknownNode("UnknownNode", "null", null, anyIfs: true);
				}
				break;
			}
			default:
				throw new InvalidOperationException(Res.GetString("Internal error."));
			}
		}
		if (choice != null)
		{
			WriteSourceBegin(choiceSource);
			CodeIdentifier.CheckValidIdentifier(choice.MemberIds[elementIndex]);
			base.RaCodeGen.ILGenForEnumMember(ilg, choice.Mapping.TypeDesc.Type, choice.MemberIds[elementIndex]);
			WriteSourceEnd(choiceSource, choice.Mapping.TypeDesc.Type);
		}
	}

	private void WriteDerivedSerializable(SerializableMapping head, SerializableMapping mapping, string source, bool isWrappedAny)
	{
		if (mapping == null)
		{
			return;
		}
		for (SerializableMapping serializableMapping = mapping.DerivedMappings; serializableMapping != null; serializableMapping = serializableMapping.NextDerivedMapping)
		{
			Label label = ilg.DefineLabel();
			Label label2 = ilg.DefineLabel();
			LocalBuilder local = ilg.GetLocal("tser");
			ilg.InitElseIf();
			ilg.Ldloc(local);
			ilg.Load(null);
			ilg.Ceq();
			ilg.Brtrue(label);
			WriteQNameEqual("tser", serializableMapping.XsiType.Name, serializableMapping.XsiType.Namespace);
			ilg.Br_S(label2);
			ilg.MarkLabel(label);
			ilg.Ldc(boolVar: true);
			ilg.MarkLabel(label2);
			ilg.AndIf();
			if (serializableMapping.Type != null)
			{
				if (head.Type.IsAssignableFrom(serializableMapping.Type))
				{
					WriteSourceBeginTyped(source, head.TypeDesc);
					MethodInfo method = typeof(XmlSerializationReader).GetMethod("ReadSerializable", CodeGenerator.InstanceBindingFlags, null, (!isWrappedAny) ? new Type[1] { typeof(IXmlSerializable) } : new Type[2]
					{
						typeof(IXmlSerializable),
						typeof(bool)
					}, null);
					ilg.Ldarg(0);
					base.RaCodeGen.ILGenForCreateInstance(ilg, serializableMapping.TypeDesc.Type, serializableMapping.TypeDesc.CannotNew, cast: false);
					if (serializableMapping.TypeDesc.CannotNew)
					{
						ilg.ConvertValue(typeof(object), typeof(IXmlSerializable));
					}
					if (isWrappedAny)
					{
						ilg.Ldc(boolVar: true);
					}
					ilg.Call(method);
					if (head.TypeDesc != null)
					{
						ilg.ConvertValue(typeof(IXmlSerializable), head.TypeDesc.Type);
					}
					WriteSourceEnd(source, head.TypeDesc.Type);
				}
				else
				{
					MethodInfo method2 = typeof(XmlSerializationReader).GetMethod("CreateBadDerivationException", CodeGenerator.InstanceBindingFlags, null, new Type[6]
					{
						typeof(string),
						typeof(string),
						typeof(string),
						typeof(string),
						typeof(string),
						typeof(string)
					}, null);
					ilg.Ldarg(0);
					ilg.Ldstr(serializableMapping.XsiType.Name);
					ilg.Ldstr(serializableMapping.XsiType.Namespace);
					ilg.Ldstr(head.XsiType.Name);
					ilg.Ldstr(head.XsiType.Namespace);
					ilg.Ldstr(serializableMapping.Type.FullName);
					ilg.Ldstr(head.Type.FullName);
					ilg.Call(method2);
					ilg.Throw();
				}
			}
			else
			{
				MethodInfo method3 = typeof(XmlSerializationReader).GetMethod("CreateMissingIXmlSerializableType", CodeGenerator.InstanceBindingFlags, null, new Type[3]
				{
					typeof(string),
					typeof(string),
					typeof(string)
				}, null);
				ilg.Ldarg(0);
				ilg.Ldstr(serializableMapping.XsiType.Name);
				ilg.Ldstr(serializableMapping.XsiType.Namespace);
				ilg.Ldstr(head.Type.FullName);
				ilg.Call(method3);
				ilg.Throw();
			}
			WriteDerivedSerializable(head, serializableMapping, source, isWrappedAny);
		}
	}

	private int WriteWhileNotLoopStart()
	{
		MethodInfo method = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		MethodInfo method2 = typeof(XmlReader).GetMethod("MoveToContent", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg.Ldarg(0);
		ilg.Call(method);
		ilg.Call(method2);
		ilg.Pop();
		int result = WriteWhileLoopStartCheck();
		ilg.WhileBegin();
		return result;
	}

	private void WriteWhileLoopEnd(int loopIndex)
	{
		WriteWhileLoopEndCheck(loopIndex);
		ilg.WhileBeginCondition();
		int intVar = 0;
		int intVar2 = 15;
		MethodInfo method = typeof(XmlSerializationReader).GetMethod("get_Reader", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		MethodInfo method2 = typeof(XmlReader).GetMethod("get_NodeType", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		Label label = ilg.DefineLabel();
		Label label2 = ilg.DefineLabel();
		ilg.Ldarg(0);
		ilg.Call(method);
		ilg.Call(method2);
		ilg.Ldc(intVar2);
		ilg.Beq(label);
		ilg.Ldarg(0);
		ilg.Call(method);
		ilg.Call(method2);
		ilg.Ldc(intVar);
		ilg.Cne();
		ilg.Br_S(label2);
		ilg.MarkLabel(label);
		ilg.Ldc(boolVar: false);
		ilg.MarkLabel(label2);
		ilg.WhileEndCondition();
		ilg.WhileEnd();
	}

	private int WriteWhileLoopStartCheck()
	{
		MethodInfo method = typeof(XmlSerializationReader).GetMethod("get_ReaderCount", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg.Ldc(0);
		ilg.Stloc(typeof(int), string.Format(CultureInfo.InvariantCulture, "whileIterations{0}", nextWhileLoopIndex));
		ilg.Ldarg(0);
		ilg.Call(method);
		ilg.Stloc(typeof(int), string.Format(CultureInfo.InvariantCulture, "readerCount{0}", nextWhileLoopIndex));
		return nextWhileLoopIndex++;
	}

	private void WriteWhileLoopEndCheck(int loopIndex)
	{
		Type type = Type.GetType("System.Int32&");
		MethodInfo method = typeof(XmlSerializationReader).GetMethod("CheckReaderCount", CodeGenerator.InstanceBindingFlags, null, new Type[2] { type, type }, null);
		ilg.Ldarg(0);
		ilg.Ldloca(ilg.GetLocal(string.Format(CultureInfo.InvariantCulture, "whileIterations{0}", loopIndex)));
		ilg.Ldloca(ilg.GetLocal(string.Format(CultureInfo.InvariantCulture, "readerCount{0}", loopIndex)));
		ilg.Call(method);
	}

	private void WriteParamsRead(int length)
	{
		LocalBuilder local = ilg.DeclareLocal(typeof(bool[]), "paramsRead");
		ilg.NewArray(typeof(bool), length);
		ilg.Stloc(local);
	}

	private void WriteCreateMapping(TypeMapping mapping, string local)
	{
		string cSharpName = mapping.TypeDesc.CSharpName;
		bool cannotNew = mapping.TypeDesc.CannotNew;
		LocalBuilder local2 = ilg.DeclareLocal(mapping.TypeDesc.Type, local);
		if (cannotNew)
		{
			ilg.BeginExceptionBlock();
		}
		base.RaCodeGen.ILGenForCreateInstance(ilg, mapping.TypeDesc.Type, mapping.TypeDesc.CannotNew, cast: true);
		ilg.Stloc(local2);
		if (cannotNew)
		{
			ilg.Leave();
			WriteCatchException(typeof(MissingMethodException));
			MethodInfo method = typeof(XmlSerializationReader).GetMethod("CreateInaccessibleConstructorException", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(string) }, null);
			ilg.Ldarg(0);
			ilg.Ldstr(cSharpName);
			ilg.Call(method);
			ilg.Throw();
			WriteCatchException(typeof(SecurityException));
			MethodInfo method2 = typeof(XmlSerializationReader).GetMethod("CreateCtorHasSecurityException", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(string) }, null);
			ilg.Ldarg(0);
			ilg.Ldstr(cSharpName);
			ilg.Call(method2);
			ilg.Throw();
			ilg.EndExceptionBlock();
		}
	}

	private void WriteCatchException(Type exceptionType)
	{
		ilg.BeginCatchBlock(exceptionType);
		ilg.Pop();
	}

	private void WriteCatchCastException(TypeDesc typeDesc, string source, string id)
	{
		WriteCatchException(typeof(InvalidCastException));
		MethodInfo method = typeof(XmlSerializationReader).GetMethod("CreateInvalidCastException", CodeGenerator.InstanceBindingFlags, null, new Type[3]
		{
			typeof(Type),
			typeof(object),
			typeof(string)
		}, null);
		ilg.Ldarg(0);
		ilg.Ldc(typeDesc.Type);
		if (source.StartsWith("GetTarget(ids[", StringComparison.Ordinal))
		{
			MethodInfo method2 = typeof(XmlSerializationReader).GetMethod("GetTarget", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(string) }, null);
			object variable = ilg.GetVariable("ids");
			ilg.Ldarg(0);
			ilg.LoadArrayElement(variable, int.Parse(source.Substring(14, source.Length - 16), CultureInfo.InvariantCulture));
			ilg.Call(method2);
		}
		else
		{
			ilg.Load(ilg.GetVariable(source));
		}
		if (id == null)
		{
			ilg.Load(null);
		}
		else if (id.StartsWith("ids[", StringComparison.Ordinal))
		{
			object variable2 = ilg.GetVariable("ids");
			ilg.LoadArrayElement(variable2, int.Parse(id.Substring(4, id.Length - 5), CultureInfo.InvariantCulture));
		}
		else
		{
			object variable3 = ilg.GetVariable(id);
			ilg.Load(variable3);
			ilg.ConvertValue(ilg.GetVariableType(variable3), typeof(string));
		}
		ilg.Call(method);
		ilg.Throw();
	}

	private void WriteArrayLocalDecl(string typeName, string variableName, string initValue, TypeDesc arrayTypeDesc)
	{
		base.RaCodeGen.WriteArrayLocalDecl(typeName, variableName, new SourceInfo(initValue, initValue, null, arrayTypeDesc.Type, ilg), arrayTypeDesc);
	}

	private void WriteCreateInstance(string source, bool ctorInaccessible, Type type)
	{
		base.RaCodeGen.WriteCreateInstance(source, ctorInaccessible, type, ilg);
	}

	private void WriteLocalDecl(string variableName, SourceInfo initValue)
	{
		base.RaCodeGen.WriteLocalDecl(variableName, initValue);
	}

	private void ILGenElseString(string elseString)
	{
		MethodInfo method = typeof(XmlSerializationReader).GetMethod("UnknownNode", CodeGenerator.InstanceBindingFlags, null, new Type[1] { typeof(object) }, null);
		MethodInfo method2 = typeof(XmlSerializationReader).GetMethod("UnknownNode", CodeGenerator.InstanceBindingFlags, null, new Type[2]
		{
			typeof(object),
			typeof(string)
		}, null);
		Match match = XmlSerializationILGen.NewRegex("UnknownNode[(]null, @[\"](?<qnames>[^\"]*)[\"][)];").Match(elseString);
		if (match.Success)
		{
			ilg.Ldarg(0);
			ilg.Load(null);
			ilg.Ldstr(match.Groups["qnames"].Value);
			ilg.Call(method2);
			return;
		}
		match = XmlSerializationILGen.NewRegex("UnknownNode[(][(]object[)](?<o>[^,]+), @[\"](?<qnames>[^\"]*)[\"][)];").Match(elseString);
		if (match.Success)
		{
			ilg.Ldarg(0);
			LocalBuilder local = ilg.GetLocal(match.Groups["o"].Value);
			ilg.Ldloc(local);
			ilg.ConvertValue(local.LocalType, typeof(object));
			ilg.Ldstr(match.Groups["qnames"].Value);
			ilg.Call(method2);
			return;
		}
		match = XmlSerializationILGen.NewRegex("UnknownNode[(][(]object[)](?<o>[^,]+), null[)];").Match(elseString);
		if (match.Success)
		{
			ilg.Ldarg(0);
			LocalBuilder local2 = ilg.GetLocal(match.Groups["o"].Value);
			ilg.Ldloc(local2);
			ilg.ConvertValue(local2.LocalType, typeof(object));
			ilg.Load(null);
			ilg.Call(method2);
			return;
		}
		match = XmlSerializationILGen.NewRegex("UnknownNode[(][(]object[)](?<o>[^)]+)[)];").Match(elseString);
		if (match.Success)
		{
			ilg.Ldarg(0);
			LocalBuilder local3 = ilg.GetLocal(match.Groups["o"].Value);
			ilg.Ldloc(local3);
			ilg.ConvertValue(local3.LocalType, typeof(object));
			ilg.Call(method);
			return;
		}
		throw CodeGenerator.NotSupported("Unexpected: " + elseString);
	}

	private void ILGenParamsReadSource(string paramsReadSource)
	{
		Match match = XmlSerializationILGen.NewRegex("paramsRead\\[(?<index>[0-9]+)\\]").Match(paramsReadSource);
		if (match.Success)
		{
			ilg.LoadArrayElement(ilg.GetLocal("paramsRead"), int.Parse(match.Groups["index"].Value, CultureInfo.InvariantCulture));
			return;
		}
		throw CodeGenerator.NotSupported("Unexpected: " + paramsReadSource);
	}

	private void ILGenParamsReadSource(string paramsReadSource, bool value)
	{
		Match match = XmlSerializationILGen.NewRegex("paramsRead\\[(?<index>[0-9]+)\\]").Match(paramsReadSource);
		if (match.Success)
		{
			ilg.StoreArrayElement(ilg.GetLocal("paramsRead"), int.Parse(match.Groups["index"].Value, CultureInfo.InvariantCulture), value);
			return;
		}
		throw CodeGenerator.NotSupported("Unexpected: " + paramsReadSource);
	}

	private void ILGenElementElseString(string elementElseString)
	{
		if (elementElseString == "throw CreateUnknownNodeException();")
		{
			MethodInfo method = typeof(XmlSerializationReader).GetMethod("CreateUnknownNodeException", CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldarg(0);
			ilg.Call(method);
			ilg.Throw();
		}
		else
		{
			if (!elementElseString.StartsWith("UnknownNode(", StringComparison.Ordinal))
			{
				throw CodeGenerator.NotSupported("Unexpected: " + elementElseString);
			}
			ILGenElseString(elementElseString);
		}
	}

	private void ILGenSet(string source, object value)
	{
		WriteSourceBegin(source);
		ilg.Load(value);
		WriteSourceEnd(source, (value == null) ? typeof(object) : value.GetType());
	}
}
