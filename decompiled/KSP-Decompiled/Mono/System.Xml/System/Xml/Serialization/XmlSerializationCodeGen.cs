using System.Collections;

namespace System.Xml.Serialization;

internal class XmlSerializationCodeGen
{
	private IndentedWriter writer;

	private int nextMethodNumber;

	private Hashtable methodNames = new Hashtable();

	private ReflectionAwareCodeGen raCodeGen;

	private TypeScope[] scopes;

	private TypeDesc stringTypeDesc;

	private TypeDesc qnameTypeDesc;

	private string access;

	private string className;

	private TypeMapping[] referencedMethods;

	private int references;

	private Hashtable generatedMethods = new Hashtable();

	internal IndentedWriter Writer => writer;

	internal int NextMethodNumber
	{
		get
		{
			return nextMethodNumber;
		}
		set
		{
			nextMethodNumber = value;
		}
	}

	internal ReflectionAwareCodeGen RaCodeGen => raCodeGen;

	internal TypeDesc StringTypeDesc => stringTypeDesc;

	internal TypeDesc QnameTypeDesc => qnameTypeDesc;

	internal string ClassName => className;

	internal string Access => access;

	internal TypeScope[] Scopes => scopes;

	internal Hashtable MethodNames => methodNames;

	internal Hashtable GeneratedMethods => generatedMethods;

	internal XmlSerializationCodeGen(IndentedWriter writer, TypeScope[] scopes, string access, string className)
	{
		this.writer = writer;
		this.scopes = scopes;
		if (scopes.Length != 0)
		{
			stringTypeDesc = scopes[0].GetTypeDesc(typeof(string));
			qnameTypeDesc = scopes[0].GetTypeDesc(typeof(XmlQualifiedName));
		}
		raCodeGen = new ReflectionAwareCodeGen(writer);
		this.className = className;
		this.access = access;
	}

	internal virtual void GenerateMethod(TypeMapping mapping)
	{
	}

	internal void GenerateReferencedMethods()
	{
		while (references > 0)
		{
			TypeMapping mapping = referencedMethods[--references];
			GenerateMethod(mapping);
		}
	}

	internal string ReferenceMapping(TypeMapping mapping)
	{
		if (!mapping.IsSoap && generatedMethods[mapping] == null)
		{
			referencedMethods = EnsureArrayIndex(referencedMethods, references);
			referencedMethods[references++] = mapping;
		}
		return (string)methodNames[mapping];
	}

	private TypeMapping[] EnsureArrayIndex(TypeMapping[] a, int index)
	{
		if (a == null)
		{
			return new TypeMapping[32];
		}
		if (index < a.Length)
		{
			return a;
		}
		TypeMapping[] array = new TypeMapping[a.Length + 32];
		Array.Copy(a, array, index);
		return array;
	}

	internal void WriteQuotedCSharpString(string value)
	{
		raCodeGen.WriteQuotedCSharpString(value);
	}

	internal void GenerateHashtableGetBegin(string privateName, string publicName)
	{
		writer.Write(typeof(Hashtable).FullName);
		writer.Write(" ");
		writer.Write(privateName);
		writer.WriteLine(" = null;");
		writer.Write("public override ");
		writer.Write(typeof(Hashtable).FullName);
		writer.Write(" ");
		writer.Write(publicName);
		writer.WriteLine(" {");
		writer.Indent++;
		writer.WriteLine("get {");
		writer.Indent++;
		writer.Write("if (");
		writer.Write(privateName);
		writer.WriteLine(" == null) {");
		writer.Indent++;
		writer.Write(typeof(Hashtable).FullName);
		writer.Write(" _tmp = new ");
		writer.Write(typeof(Hashtable).FullName);
		writer.WriteLine("();");
	}

	internal void GenerateHashtableGetEnd(string privateName)
	{
		writer.Write("if (");
		writer.Write(privateName);
		writer.Write(" == null) ");
		writer.Write(privateName);
		writer.WriteLine(" = _tmp;");
		writer.Indent--;
		writer.WriteLine("}");
		writer.Write("return ");
		writer.Write(privateName);
		writer.WriteLine(";");
		writer.Indent--;
		writer.WriteLine("}");
		writer.Indent--;
		writer.WriteLine("}");
	}

	internal void GeneratePublicMethods(string privateName, string publicName, string[] methods, XmlMapping[] xmlMappings)
	{
		GenerateHashtableGetBegin(privateName, publicName);
		if (methods != null && methods.Length != 0 && xmlMappings != null && xmlMappings.Length == methods.Length)
		{
			for (int i = 0; i < methods.Length; i++)
			{
				if (methods[i] != null)
				{
					writer.Write("_tmp[");
					WriteQuotedCSharpString(xmlMappings[i].Key);
					writer.Write("] = ");
					WriteQuotedCSharpString(methods[i]);
					writer.WriteLine(";");
				}
			}
		}
		GenerateHashtableGetEnd(privateName);
	}

	internal void GenerateSupportedTypes(Type[] types)
	{
		writer.Write("public override ");
		writer.Write(typeof(bool).FullName);
		writer.Write(" CanSerialize(");
		writer.Write(typeof(Type).FullName);
		writer.WriteLine(" type) {");
		writer.Indent++;
		Hashtable hashtable = new Hashtable();
		foreach (Type type in types)
		{
			if (!(type == null) && (type.IsPublic || type.IsNestedPublic) && hashtable[type] == null && !DynamicAssemblies.IsTypeDynamic(type) && !type.IsGenericType && (!type.ContainsGenericParameters || !DynamicAssemblies.IsTypeDynamic(type.GetGenericArguments())))
			{
				hashtable[type] = type;
				writer.Write("if (type == typeof(");
				writer.Write(CodeIdentifier.GetCSharpName(type));
				writer.WriteLine(")) return true;");
			}
		}
		writer.WriteLine("return false;");
		writer.Indent--;
		writer.WriteLine("}");
	}

	internal string GenerateBaseSerializer(string baseSerializer, string readerClass, string writerClass, CodeIdentifiers classes)
	{
		baseSerializer = CodeIdentifier.MakeValid(baseSerializer);
		baseSerializer = classes.AddUnique(baseSerializer, baseSerializer);
		writer.WriteLine();
		writer.Write("public abstract class ");
		writer.Write(CodeIdentifier.GetCSharpName(baseSerializer));
		writer.Write(" : ");
		writer.Write(typeof(XmlSerializer).FullName);
		writer.WriteLine(" {");
		writer.Indent++;
		writer.Write("protected override ");
		writer.Write(typeof(XmlSerializationReader).FullName);
		writer.WriteLine(" CreateReader() {");
		writer.Indent++;
		writer.Write("return new ");
		writer.Write(readerClass);
		writer.WriteLine("();");
		writer.Indent--;
		writer.WriteLine("}");
		writer.Write("protected override ");
		writer.Write(typeof(XmlSerializationWriter).FullName);
		writer.WriteLine(" CreateWriter() {");
		writer.Indent++;
		writer.Write("return new ");
		writer.Write(writerClass);
		writer.WriteLine("();");
		writer.Indent--;
		writer.WriteLine("}");
		writer.Indent--;
		writer.WriteLine("}");
		return baseSerializer;
	}

	internal string GenerateTypedSerializer(string readMethod, string writeMethod, XmlMapping mapping, CodeIdentifiers classes, string baseSerializer, string readerClass, string writerClass)
	{
		string text = CodeIdentifier.MakeValid(Accessor.UnescapeName(mapping.Accessor.Mapping.TypeDesc.Name));
		text = classes.AddUnique(text + "Serializer", mapping);
		writer.WriteLine();
		writer.Write("public sealed class ");
		writer.Write(CodeIdentifier.GetCSharpName(text));
		writer.Write(" : ");
		writer.Write(baseSerializer);
		writer.WriteLine(" {");
		writer.Indent++;
		writer.WriteLine();
		writer.Write("public override ");
		writer.Write(typeof(bool).FullName);
		writer.Write(" CanDeserialize(");
		writer.Write(typeof(XmlReader).FullName);
		writer.WriteLine(" xmlReader) {");
		writer.Indent++;
		if (mapping.Accessor.Any)
		{
			writer.WriteLine("return true;");
		}
		else
		{
			writer.Write("return xmlReader.IsStartElement(");
			WriteQuotedCSharpString(mapping.Accessor.Name);
			writer.Write(", ");
			WriteQuotedCSharpString(mapping.Accessor.Namespace);
			writer.WriteLine(");");
		}
		writer.Indent--;
		writer.WriteLine("}");
		if (writeMethod != null)
		{
			writer.WriteLine();
			writer.Write("protected override void Serialize(object objectToSerialize, ");
			writer.Write(typeof(XmlSerializationWriter).FullName);
			writer.WriteLine(" writer) {");
			writer.Indent++;
			writer.Write("((");
			writer.Write(writerClass);
			writer.Write(")writer).");
			writer.Write(writeMethod);
			writer.Write("(");
			if (mapping is XmlMembersMapping)
			{
				writer.Write("(object[])");
			}
			writer.WriteLine("objectToSerialize);");
			writer.Indent--;
			writer.WriteLine("}");
		}
		if (readMethod != null)
		{
			writer.WriteLine();
			writer.Write("protected override object Deserialize(");
			writer.Write(typeof(XmlSerializationReader).FullName);
			writer.WriteLine(" reader) {");
			writer.Indent++;
			writer.Write("return ((");
			writer.Write(readerClass);
			writer.Write(")reader).");
			writer.Write(readMethod);
			writer.WriteLine("();");
			writer.Indent--;
			writer.WriteLine("}");
		}
		writer.Indent--;
		writer.WriteLine("}");
		return text;
	}

	private void GenerateTypedSerializers(Hashtable serializers)
	{
		string privateName = "typedSerializers";
		GenerateHashtableGetBegin(privateName, "TypedSerializers");
		foreach (string key in serializers.Keys)
		{
			writer.Write("_tmp.Add(");
			WriteQuotedCSharpString(key);
			writer.Write(", new ");
			writer.Write((string)serializers[key]);
			writer.WriteLine("());");
		}
		GenerateHashtableGetEnd("typedSerializers");
	}

	private void GenerateGetSerializer(Hashtable serializers, XmlMapping[] xmlMappings)
	{
		writer.Write("public override ");
		writer.Write(typeof(XmlSerializer).FullName);
		writer.Write(" GetSerializer(");
		writer.Write(typeof(Type).FullName);
		writer.WriteLine(" type) {");
		writer.Indent++;
		for (int i = 0; i < xmlMappings.Length; i++)
		{
			if (xmlMappings[i] is XmlTypeMapping)
			{
				Type type = xmlMappings[i].Accessor.Mapping.TypeDesc.Type;
				if (!(type == null) && (type.IsPublic || type.IsNestedPublic) && !DynamicAssemblies.IsTypeDynamic(type) && !type.IsGenericType && (!type.ContainsGenericParameters || !DynamicAssemblies.IsTypeDynamic(type.GetGenericArguments())))
				{
					writer.Write("if (type == typeof(");
					writer.Write(CodeIdentifier.GetCSharpName(type));
					writer.Write(")) return new ");
					writer.Write((string)serializers[xmlMappings[i].Key]);
					writer.WriteLine("();");
				}
			}
		}
		writer.WriteLine("return null;");
		writer.Indent--;
		writer.WriteLine("}");
	}

	internal void GenerateSerializerContract(string className, XmlMapping[] xmlMappings, Type[] types, string readerType, string[] readMethods, string writerType, string[] writerMethods, Hashtable serializers)
	{
		writer.WriteLine();
		writer.Write("public class XmlSerializerContract : global::");
		writer.Write(typeof(XmlSerializerImplementation).FullName);
		writer.WriteLine(" {");
		writer.Indent++;
		writer.Write("public override global::");
		writer.Write(typeof(XmlSerializationReader).FullName);
		writer.Write(" Reader { get { return new ");
		writer.Write(readerType);
		writer.WriteLine("(); } }");
		writer.Write("public override global::");
		writer.Write(typeof(XmlSerializationWriter).FullName);
		writer.Write(" Writer { get { return new ");
		writer.Write(writerType);
		writer.WriteLine("(); } }");
		GeneratePublicMethods("readMethods", "ReadMethods", readMethods, xmlMappings);
		GeneratePublicMethods("writeMethods", "WriteMethods", writerMethods, xmlMappings);
		GenerateTypedSerializers(serializers);
		GenerateSupportedTypes(types);
		GenerateGetSerializer(serializers, xmlMappings);
		writer.Indent--;
		writer.WriteLine("}");
	}

	internal static bool IsWildcard(SpecialMapping mapping)
	{
		if (mapping is SerializableMapping)
		{
			return ((SerializableMapping)mapping).IsAny;
		}
		return mapping.TypeDesc.CanBeElementValue;
	}
}
