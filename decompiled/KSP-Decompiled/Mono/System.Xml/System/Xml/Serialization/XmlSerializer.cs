using System.CodeDom.Compiler;
using System.Collections;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Security.Permissions;
using System.Security.Policy;
using System.Threading;

namespace System.Xml.Serialization;

public class XmlSerializer
{
	private class XmlSerializerMappingKey
	{
		public XmlMapping Mapping;

		public XmlSerializerMappingKey(XmlMapping mapping)
		{
			Mapping = mapping;
		}

		public override bool Equals(object obj)
		{
			if (!(obj is XmlSerializerMappingKey xmlSerializerMappingKey))
			{
				return false;
			}
			if (Mapping.Key != xmlSerializerMappingKey.Mapping.Key)
			{
				return false;
			}
			if (Mapping.ElementName != xmlSerializerMappingKey.Mapping.ElementName)
			{
				return false;
			}
			if (Mapping.Namespace != xmlSerializerMappingKey.Mapping.Namespace)
			{
				return false;
			}
			if (Mapping.IsSoap != xmlSerializerMappingKey.Mapping.IsSoap)
			{
				return false;
			}
			return true;
		}

		public override int GetHashCode()
		{
			int num = ((!Mapping.IsSoap) ? 1 : 0);
			if (Mapping.Key != null)
			{
				num ^= Mapping.Key.GetHashCode();
			}
			if (Mapping.ElementName != null)
			{
				num ^= Mapping.ElementName.GetHashCode();
			}
			if (Mapping.Namespace != null)
			{
				num ^= Mapping.Namespace.GetHashCode();
			}
			return num;
		}
	}

	private TempAssembly tempAssembly;

	private bool typedSerializer;

	private Type primitiveType;

	private XmlMapping mapping;

	private XmlDeserializationEvents events;

	private static TempAssemblyCache cache = new TempAssemblyCache();

	private static volatile XmlSerializerNamespaces defaultNamespaces;

	private static Hashtable xmlSerializerTable = new Hashtable();

	private static XmlSerializerNamespaces DefaultNamespaces
	{
		get
		{
			if (defaultNamespaces == null)
			{
				XmlSerializerNamespaces xmlSerializerNamespaces = new XmlSerializerNamespaces();
				xmlSerializerNamespaces.AddInternal("xsi", "http://www.w3.org/2001/XMLSchema-instance");
				xmlSerializerNamespaces.AddInternal("xsd", "http://www.w3.org/2001/XMLSchema");
				if (defaultNamespaces == null)
				{
					defaultNamespaces = xmlSerializerNamespaces;
				}
			}
			return defaultNamespaces;
		}
	}

	public event XmlNodeEventHandler UnknownNode
	{
		add
		{
			ref XmlDeserializationEvents reference = ref events;
			reference.OnUnknownNode = (XmlNodeEventHandler)Delegate.Combine(reference.OnUnknownNode, value);
		}
		remove
		{
			ref XmlDeserializationEvents reference = ref events;
			reference.OnUnknownNode = (XmlNodeEventHandler)Delegate.Remove(reference.OnUnknownNode, value);
		}
	}

	public event XmlAttributeEventHandler UnknownAttribute
	{
		add
		{
			ref XmlDeserializationEvents reference = ref events;
			reference.OnUnknownAttribute = (XmlAttributeEventHandler)Delegate.Combine(reference.OnUnknownAttribute, value);
		}
		remove
		{
			ref XmlDeserializationEvents reference = ref events;
			reference.OnUnknownAttribute = (XmlAttributeEventHandler)Delegate.Remove(reference.OnUnknownAttribute, value);
		}
	}

	public event XmlElementEventHandler UnknownElement
	{
		add
		{
			ref XmlDeserializationEvents reference = ref events;
			reference.OnUnknownElement = (XmlElementEventHandler)Delegate.Combine(reference.OnUnknownElement, value);
		}
		remove
		{
			ref XmlDeserializationEvents reference = ref events;
			reference.OnUnknownElement = (XmlElementEventHandler)Delegate.Remove(reference.OnUnknownElement, value);
		}
	}

	public event UnreferencedObjectEventHandler UnreferencedObject
	{
		add
		{
			ref XmlDeserializationEvents reference = ref events;
			reference.OnUnreferencedObject = (UnreferencedObjectEventHandler)Delegate.Combine(reference.OnUnreferencedObject, value);
		}
		remove
		{
			ref XmlDeserializationEvents reference = ref events;
			reference.OnUnreferencedObject = (UnreferencedObjectEventHandler)Delegate.Remove(reference.OnUnreferencedObject, value);
		}
	}

	protected XmlSerializer()
	{
	}

	public XmlSerializer(Type type, XmlAttributeOverrides overrides, Type[] extraTypes, XmlRootAttribute root, string defaultNamespace)
		: this(type, overrides, extraTypes, root, defaultNamespace, null)
	{
	}

	public XmlSerializer(Type type, XmlRootAttribute root)
		: this(type, null, new Type[0], root, null, null)
	{
	}

	public XmlSerializer(Type type, Type[] extraTypes)
		: this(type, null, extraTypes, null, null, null)
	{
	}

	public XmlSerializer(Type type, XmlAttributeOverrides overrides)
		: this(type, overrides, new Type[0], null, null, null)
	{
	}

	public XmlSerializer(XmlTypeMapping xmlTypeMapping)
	{
		tempAssembly = GenerateTempAssembly(xmlTypeMapping);
		mapping = xmlTypeMapping;
	}

	public XmlSerializer(Type type)
		: this(type, (string)null)
	{
	}

	public XmlSerializer(Type type, string defaultNamespace)
	{
		if (type == null)
		{
			throw new ArgumentNullException("type");
		}
		mapping = GetKnownMapping(type, defaultNamespace);
		if (mapping != null)
		{
			primitiveType = type;
			return;
		}
		tempAssembly = cache[defaultNamespace, type];
		if (tempAssembly == null)
		{
			lock (cache)
			{
				tempAssembly = cache[defaultNamespace, type];
				if (tempAssembly == null)
				{
					XmlSerializerImplementation contract;
					Assembly assembly = TempAssembly.LoadGeneratedAssembly(type, defaultNamespace, out contract);
					if (assembly == null)
					{
						XmlReflectionImporter xmlReflectionImporter = new XmlReflectionImporter(defaultNamespace);
						mapping = xmlReflectionImporter.ImportTypeMapping(type, null, defaultNamespace);
						tempAssembly = GenerateTempAssembly(mapping, type, defaultNamespace);
					}
					else
					{
						mapping = XmlReflectionImporter.GetTopLevelMapping(type, defaultNamespace);
						tempAssembly = new TempAssembly(new XmlMapping[1] { mapping }, assembly, contract);
					}
				}
				cache.Add(defaultNamespace, type, tempAssembly);
			}
		}
		if (mapping == null)
		{
			mapping = XmlReflectionImporter.GetTopLevelMapping(type, defaultNamespace);
		}
	}

	public XmlSerializer(Type type, XmlAttributeOverrides overrides, Type[] extraTypes, XmlRootAttribute root, string defaultNamespace, string location)
		: this(type, overrides, extraTypes, root, defaultNamespace, location, null)
	{
	}

	[Obsolete("This method is obsolete and will be removed in a future release of the .NET Framework. Please use a XmlSerializer constructor overload which does not take an Evidence parameter. See http://go2.microsoft.com/fwlink/?LinkId=131738 for more information.")]
	public XmlSerializer(Type type, XmlAttributeOverrides overrides, Type[] extraTypes, XmlRootAttribute root, string defaultNamespace, string location, Evidence evidence)
	{
		if (type == null)
		{
			throw new ArgumentNullException("type");
		}
		XmlReflectionImporter xmlReflectionImporter = new XmlReflectionImporter(overrides, defaultNamespace);
		if (extraTypes != null)
		{
			for (int i = 0; i < extraTypes.Length; i++)
			{
				xmlReflectionImporter.IncludeType(extraTypes[i]);
			}
		}
		mapping = xmlReflectionImporter.ImportTypeMapping(type, root, defaultNamespace);
		if (location != null || evidence != null)
		{
			DemandForUserLocationOrEvidence();
		}
		tempAssembly = GenerateTempAssembly(mapping, type, defaultNamespace, location, evidence);
	}

	[PermissionSet(SecurityAction.Demand, Name = "FullTrust")]
	private void DemandForUserLocationOrEvidence()
	{
	}

	internal static TempAssembly GenerateTempAssembly(XmlMapping xmlMapping)
	{
		return GenerateTempAssembly(xmlMapping, null, null);
	}

	internal static TempAssembly GenerateTempAssembly(XmlMapping xmlMapping, Type type, string defaultNamespace)
	{
		if (xmlMapping == null)
		{
			throw new ArgumentNullException("xmlMapping");
		}
		return new TempAssembly(new XmlMapping[1] { xmlMapping }, new Type[1] { type }, defaultNamespace, null, null);
	}

	internal static TempAssembly GenerateTempAssembly(XmlMapping xmlMapping, Type type, string defaultNamespace, string location, Evidence evidence)
	{
		return new TempAssembly(new XmlMapping[1] { xmlMapping }, new Type[1] { type }, defaultNamespace, location, evidence);
	}

	public void Serialize(TextWriter textWriter, object o)
	{
		Serialize(textWriter, o, null);
	}

	public void Serialize(TextWriter textWriter, object o, XmlSerializerNamespaces namespaces)
	{
		XmlTextWriter xmlTextWriter = new XmlTextWriter(textWriter);
		xmlTextWriter.Formatting = Formatting.Indented;
		xmlTextWriter.Indentation = 2;
		Serialize(xmlTextWriter, o, namespaces);
	}

	public void Serialize(Stream stream, object o)
	{
		Serialize(stream, o, null);
	}

	public void Serialize(Stream stream, object o, XmlSerializerNamespaces namespaces)
	{
		XmlTextWriter xmlTextWriter = new XmlTextWriter(stream, null);
		xmlTextWriter.Formatting = Formatting.Indented;
		xmlTextWriter.Indentation = 2;
		Serialize(xmlTextWriter, o, namespaces);
	}

	public void Serialize(XmlWriter xmlWriter, object o)
	{
		Serialize(xmlWriter, o, null);
	}

	public void Serialize(XmlWriter xmlWriter, object o, XmlSerializerNamespaces namespaces)
	{
		Serialize(xmlWriter, o, namespaces, null);
	}

	public void Serialize(XmlWriter xmlWriter, object o, XmlSerializerNamespaces namespaces, string encodingStyle)
	{
		Serialize(xmlWriter, o, namespaces, encodingStyle, null);
	}

	public void Serialize(XmlWriter xmlWriter, object o, XmlSerializerNamespaces namespaces, string encodingStyle, string id)
	{
		try
		{
			if (primitiveType != null)
			{
				if (encodingStyle != null && encodingStyle.Length > 0)
				{
					throw new InvalidOperationException(Res.GetString("The encoding style '{0}' is not valid for this call because this XmlSerializer instance does not support encoding. Use the SoapReflectionImporter to initialize an XmlSerializer that supports encoding.", encodingStyle));
				}
				SerializePrimitive(xmlWriter, o, namespaces);
			}
			else if (tempAssembly == null || typedSerializer)
			{
				XmlSerializationWriter xmlSerializationWriter = CreateWriter();
				xmlSerializationWriter.Init(xmlWriter, (namespaces == null || namespaces.Count == 0) ? DefaultNamespaces : namespaces, encodingStyle, id, tempAssembly);
				try
				{
					Serialize(o, xmlSerializationWriter);
				}
				finally
				{
					xmlSerializationWriter.Dispose();
				}
			}
			else
			{
				tempAssembly.InvokeWriter(mapping, xmlWriter, o, (namespaces == null || namespaces.Count == 0) ? DefaultNamespaces : namespaces, encodingStyle, id);
			}
		}
		catch (Exception innerException)
		{
			if (innerException is ThreadAbortException || innerException is StackOverflowException || innerException is OutOfMemoryException)
			{
				throw;
			}
			if (innerException is TargetInvocationException)
			{
				innerException = innerException.InnerException;
			}
			throw new InvalidOperationException(Res.GetString("There was an error generating the XML document."), innerException);
		}
		xmlWriter.Flush();
	}

	public object Deserialize(Stream stream)
	{
		XmlTextReader xmlTextReader = new XmlTextReader(stream);
		xmlTextReader.WhitespaceHandling = WhitespaceHandling.Significant;
		xmlTextReader.Normalization = true;
		xmlTextReader.XmlResolver = null;
		return Deserialize(xmlTextReader, null);
	}

	public object Deserialize(TextReader textReader)
	{
		XmlTextReader xmlTextReader = new XmlTextReader(textReader);
		xmlTextReader.WhitespaceHandling = WhitespaceHandling.Significant;
		xmlTextReader.Normalization = true;
		xmlTextReader.XmlResolver = null;
		return Deserialize(xmlTextReader, null);
	}

	public object Deserialize(XmlReader xmlReader)
	{
		return Deserialize(xmlReader, null);
	}

	public object Deserialize(XmlReader xmlReader, XmlDeserializationEvents events)
	{
		return Deserialize(xmlReader, null, events);
	}

	public object Deserialize(XmlReader xmlReader, string encodingStyle)
	{
		return Deserialize(xmlReader, encodingStyle, events);
	}

	public object Deserialize(XmlReader xmlReader, string encodingStyle, XmlDeserializationEvents events)
	{
		events.sender = this;
		try
		{
			if (primitiveType != null)
			{
				if (encodingStyle != null && encodingStyle.Length > 0)
				{
					throw new InvalidOperationException(Res.GetString("The encoding style '{0}' is not valid for this call because this XmlSerializer instance does not support encoding. Use the SoapReflectionImporter to initialize an XmlSerializer that supports encoding.", encodingStyle));
				}
				return DeserializePrimitive(xmlReader, events);
			}
			if (tempAssembly == null || typedSerializer)
			{
				XmlSerializationReader xmlSerializationReader = CreateReader();
				xmlSerializationReader.Init(xmlReader, events, encodingStyle, tempAssembly);
				try
				{
					return Deserialize(xmlSerializationReader);
				}
				finally
				{
					xmlSerializationReader.Dispose();
				}
			}
			return tempAssembly.InvokeReader(mapping, xmlReader, events, encodingStyle);
		}
		catch (Exception innerException)
		{
			if (innerException is ThreadAbortException || innerException is StackOverflowException || innerException is OutOfMemoryException)
			{
				throw;
			}
			if (innerException is TargetInvocationException)
			{
				innerException = innerException.InnerException;
			}
			if (xmlReader is IXmlLineInfo)
			{
				IXmlLineInfo xmlLineInfo = (IXmlLineInfo)xmlReader;
				throw new InvalidOperationException(Res.GetString("There is an error in XML document ({0}, {1}).", xmlLineInfo.LineNumber.ToString(CultureInfo.InvariantCulture), xmlLineInfo.LinePosition.ToString(CultureInfo.InvariantCulture)), innerException);
			}
			throw new InvalidOperationException(Res.GetString("There is an error in the XML document."), innerException);
		}
	}

	public virtual bool CanDeserialize(XmlReader xmlReader)
	{
		if (primitiveType != null)
		{
			TypeDesc typeDesc = (TypeDesc)TypeScope.PrimtiveTypes[primitiveType];
			return xmlReader.IsStartElement(typeDesc.DataType.Name, string.Empty);
		}
		if (tempAssembly != null)
		{
			return tempAssembly.CanRead(mapping, xmlReader);
		}
		return false;
	}

	[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
	public static XmlSerializer[] FromMappings(XmlMapping[] mappings)
	{
		return FromMappings(mappings, (Type)null);
	}

	[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
	public static XmlSerializer[] FromMappings(XmlMapping[] mappings, Type type)
	{
		if (mappings == null || mappings.Length == 0)
		{
			return new XmlSerializer[0];
		}
		XmlSerializerImplementation contract = null;
		Assembly obj = ((type == null) ? null : TempAssembly.LoadGeneratedAssembly(type, null, out contract));
		TempAssembly tempAssembly = null;
		if (obj == null)
		{
			if (XmlMapping.IsShallow(mappings))
			{
				return new XmlSerializer[0];
			}
			if (type == null)
			{
				tempAssembly = new TempAssembly(mappings, new Type[1] { type }, null, null, null);
				XmlSerializer[] array = new XmlSerializer[mappings.Length];
				contract = tempAssembly.Contract;
				for (int i = 0; i < array.Length; i++)
				{
					array[i] = (XmlSerializer)contract.TypedSerializers[mappings[i].Key];
					array[i].SetTempAssembly(tempAssembly, mappings[i]);
				}
				return array;
			}
			return GetSerializersFromCache(mappings, type);
		}
		XmlSerializer[] array2 = new XmlSerializer[mappings.Length];
		for (int j = 0; j < array2.Length; j++)
		{
			array2[j] = (XmlSerializer)contract.TypedSerializers[mappings[j].Key];
		}
		return array2;
	}

	private static XmlSerializer[] GetSerializersFromCache(XmlMapping[] mappings, Type type)
	{
		XmlSerializer[] array = new XmlSerializer[mappings.Length];
		Hashtable hashtable = null;
		lock (xmlSerializerTable)
		{
			hashtable = xmlSerializerTable[type] as Hashtable;
			if (hashtable == null)
			{
				hashtable = new Hashtable();
				xmlSerializerTable[type] = hashtable;
			}
		}
		lock (hashtable)
		{
			Hashtable hashtable2 = new Hashtable();
			for (int i = 0; i < mappings.Length; i++)
			{
				XmlSerializerMappingKey key = new XmlSerializerMappingKey(mappings[i]);
				array[i] = hashtable[key] as XmlSerializer;
				if (array[i] == null)
				{
					hashtable2.Add(key, i);
				}
			}
			if (hashtable2.Count > 0)
			{
				XmlMapping[] array2 = new XmlMapping[hashtable2.Count];
				int num = 0;
				foreach (XmlSerializerMappingKey key2 in hashtable2.Keys)
				{
					array2[num++] = key2.Mapping;
				}
				TempAssembly tempAssembly = new TempAssembly(array2, new Type[1] { type }, null, null, null);
				XmlSerializerImplementation contract = tempAssembly.Contract;
				foreach (XmlSerializerMappingKey key3 in hashtable2.Keys)
				{
					num = (int)hashtable2[key3];
					array[num] = (XmlSerializer)contract.TypedSerializers[key3.Mapping.Key];
					array[num].SetTempAssembly(tempAssembly, key3.Mapping);
					hashtable[key3] = array[num];
				}
			}
		}
		return array;
	}

	[Obsolete("This method is obsolete and will be removed in a future release of the .NET Framework. Please use an overload of FromMappings which does not take an Evidence parameter. See http://go2.microsoft.com/fwlink/?LinkId=131738 for more information.")]
	[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
	public static XmlSerializer[] FromMappings(XmlMapping[] mappings, Evidence evidence)
	{
		if (mappings == null || mappings.Length == 0)
		{
			return new XmlSerializer[0];
		}
		if (XmlMapping.IsShallow(mappings))
		{
			return new XmlSerializer[0];
		}
		XmlSerializerImplementation contract = new TempAssembly(mappings, new Type[0], null, null, evidence).Contract;
		XmlSerializer[] array = new XmlSerializer[mappings.Length];
		for (int i = 0; i < array.Length; i++)
		{
			array[i] = (XmlSerializer)contract.TypedSerializers[mappings[i].Key];
		}
		return array;
	}

	[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
	public static Assembly GenerateSerializer(Type[] types, XmlMapping[] mappings)
	{
		CompilerParameters compilerParameters = new CompilerParameters();
		compilerParameters.TempFiles = new TempFileCollection();
		compilerParameters.GenerateInMemory = false;
		compilerParameters.IncludeDebugInformation = false;
		return GenerateSerializer(types, mappings, compilerParameters);
	}

	[PermissionSet(SecurityAction.Demand, Name = "FullTrust")]
	public static Assembly GenerateSerializer(Type[] types, XmlMapping[] mappings, CompilerParameters parameters)
	{
		if (types == null || types.Length == 0)
		{
			return null;
		}
		if (mappings == null)
		{
			throw new ArgumentNullException("mappings");
		}
		if (XmlMapping.IsShallow(mappings))
		{
			throw new InvalidOperationException(Res.GetString("This mapping was not crated by reflection importer and cannot be used in this context."));
		}
		Assembly assembly = null;
		foreach (Type type in types)
		{
			if (DynamicAssemblies.IsTypeDynamic(type))
			{
				throw new InvalidOperationException(Res.GetString("Cannot pre-generate serialization assembly for type '{0}'. Pre-generation of serialization assemblies is not supported for dynamic types. Save the assembly and load it from disk to use it with XmlSerialization.", type.FullName));
			}
			if (assembly == null)
			{
				assembly = type.Assembly;
			}
			else if (type.Assembly != assembly)
			{
				throw new ArgumentException(Res.GetString("Cannot pre-generate serializer for multiple assemblies. Type '{0}' does not belong to assembly {1}.", type.FullName, assembly.Location), "types");
			}
		}
		return TempAssembly.GenerateAssembly(mappings, types, null, null, XmlSerializerCompilerParameters.Create(parameters, needTempDirAccess: true), assembly, new Hashtable());
	}

	public static XmlSerializer[] FromTypes(Type[] types)
	{
		if (types == null)
		{
			return new XmlSerializer[0];
		}
		XmlReflectionImporter xmlReflectionImporter = new XmlReflectionImporter();
		XmlTypeMapping[] array = new XmlTypeMapping[types.Length];
		for (int i = 0; i < types.Length; i++)
		{
			array[i] = xmlReflectionImporter.ImportTypeMapping(types[i]);
		}
		return FromMappings(array);
	}

	[PermissionSet(SecurityAction.Demand, Name = "FullTrust")]
	public static string GetXmlSerializerAssemblyName(Type type)
	{
		return GetXmlSerializerAssemblyName(type, null);
	}

	[PermissionSet(SecurityAction.Demand, Name = "FullTrust")]
	public static string GetXmlSerializerAssemblyName(Type type, string defaultNamespace)
	{
		if (type == null)
		{
			throw new ArgumentNullException("type");
		}
		return Compiler.GetTempAssemblyName(type.Assembly.GetName(), defaultNamespace);
	}

	protected virtual XmlSerializationReader CreateReader()
	{
		throw new NotImplementedException();
	}

	protected virtual object Deserialize(XmlSerializationReader reader)
	{
		throw new NotImplementedException();
	}

	protected virtual XmlSerializationWriter CreateWriter()
	{
		throw new NotImplementedException();
	}

	protected virtual void Serialize(object o, XmlSerializationWriter writer)
	{
		throw new NotImplementedException();
	}

	internal void SetTempAssembly(TempAssembly tempAssembly, XmlMapping mapping)
	{
		this.tempAssembly = tempAssembly;
		this.mapping = mapping;
		typedSerializer = true;
	}

	private static XmlTypeMapping GetKnownMapping(Type type, string ns)
	{
		if (ns != null && ns != string.Empty)
		{
			return null;
		}
		TypeDesc typeDesc = (TypeDesc)TypeScope.PrimtiveTypes[type];
		if (typeDesc == null)
		{
			return null;
		}
		ElementAccessor elementAccessor = new ElementAccessor();
		elementAccessor.Name = typeDesc.DataType.Name;
		XmlTypeMapping xmlTypeMapping = new XmlTypeMapping(null, elementAccessor);
		xmlTypeMapping.SetKeyInternal(XmlMapping.GenerateKey(type, null, null));
		return xmlTypeMapping;
	}

	private void SerializePrimitive(XmlWriter xmlWriter, object o, XmlSerializerNamespaces namespaces)
	{
		XmlSerializationPrimitiveWriter xmlSerializationPrimitiveWriter = new XmlSerializationPrimitiveWriter();
		xmlSerializationPrimitiveWriter.Init(xmlWriter, namespaces, null, null, null);
		switch (Type.GetTypeCode(primitiveType))
		{
		case TypeCode.String:
			xmlSerializationPrimitiveWriter.Write_string(o);
			return;
		case TypeCode.Int32:
			xmlSerializationPrimitiveWriter.Write_int(o);
			return;
		case TypeCode.Boolean:
			xmlSerializationPrimitiveWriter.Write_boolean(o);
			return;
		case TypeCode.Int16:
			xmlSerializationPrimitiveWriter.Write_short(o);
			return;
		case TypeCode.Int64:
			xmlSerializationPrimitiveWriter.Write_long(o);
			return;
		case TypeCode.Single:
			xmlSerializationPrimitiveWriter.Write_float(o);
			return;
		case TypeCode.Double:
			xmlSerializationPrimitiveWriter.Write_double(o);
			return;
		case TypeCode.Decimal:
			xmlSerializationPrimitiveWriter.Write_decimal(o);
			return;
		case TypeCode.DateTime:
			xmlSerializationPrimitiveWriter.Write_dateTime(o);
			return;
		case TypeCode.Char:
			xmlSerializationPrimitiveWriter.Write_char(o);
			return;
		case TypeCode.Byte:
			xmlSerializationPrimitiveWriter.Write_unsignedByte(o);
			return;
		case TypeCode.SByte:
			xmlSerializationPrimitiveWriter.Write_byte(o);
			return;
		case TypeCode.UInt16:
			xmlSerializationPrimitiveWriter.Write_unsignedShort(o);
			return;
		case TypeCode.UInt32:
			xmlSerializationPrimitiveWriter.Write_unsignedInt(o);
			return;
		case TypeCode.UInt64:
			xmlSerializationPrimitiveWriter.Write_unsignedLong(o);
			return;
		}
		if (primitiveType == typeof(XmlQualifiedName))
		{
			xmlSerializationPrimitiveWriter.Write_QName(o);
			return;
		}
		if (primitiveType == typeof(byte[]))
		{
			xmlSerializationPrimitiveWriter.Write_base64Binary(o);
			return;
		}
		if (primitiveType == typeof(Guid))
		{
			xmlSerializationPrimitiveWriter.Write_guid(o);
			return;
		}
		if (primitiveType == typeof(TimeSpan))
		{
			xmlSerializationPrimitiveWriter.Write_TimeSpan(o);
			return;
		}
		throw new InvalidOperationException(Res.GetString("The type {0} was not expected. Use the XmlInclude or SoapInclude attribute to specify types that are not known statically.", primitiveType.FullName));
	}

	private object DeserializePrimitive(XmlReader xmlReader, XmlDeserializationEvents events)
	{
		XmlSerializationPrimitiveReader xmlSerializationPrimitiveReader = new XmlSerializationPrimitiveReader();
		xmlSerializationPrimitiveReader.Init(xmlReader, events, null, null);
		switch (Type.GetTypeCode(primitiveType))
		{
		case TypeCode.String:
			return xmlSerializationPrimitiveReader.Read_string();
		case TypeCode.Int32:
			return xmlSerializationPrimitiveReader.Read_int();
		case TypeCode.Boolean:
			return xmlSerializationPrimitiveReader.Read_boolean();
		case TypeCode.Int16:
			return xmlSerializationPrimitiveReader.Read_short();
		case TypeCode.Int64:
			return xmlSerializationPrimitiveReader.Read_long();
		case TypeCode.Single:
			return xmlSerializationPrimitiveReader.Read_float();
		case TypeCode.Double:
			return xmlSerializationPrimitiveReader.Read_double();
		case TypeCode.Decimal:
			return xmlSerializationPrimitiveReader.Read_decimal();
		case TypeCode.DateTime:
			return xmlSerializationPrimitiveReader.Read_dateTime();
		case TypeCode.Char:
			return xmlSerializationPrimitiveReader.Read_char();
		case TypeCode.Byte:
			return xmlSerializationPrimitiveReader.Read_unsignedByte();
		case TypeCode.SByte:
			return xmlSerializationPrimitiveReader.Read_byte();
		case TypeCode.UInt16:
			return xmlSerializationPrimitiveReader.Read_unsignedShort();
		case TypeCode.UInt32:
			return xmlSerializationPrimitiveReader.Read_unsignedInt();
		case TypeCode.UInt64:
			return xmlSerializationPrimitiveReader.Read_unsignedLong();
		default:
			if (primitiveType == typeof(XmlQualifiedName))
			{
				return xmlSerializationPrimitiveReader.Read_QName();
			}
			if (primitiveType == typeof(byte[]))
			{
				return xmlSerializationPrimitiveReader.Read_base64Binary();
			}
			if (primitiveType == typeof(Guid))
			{
				return xmlSerializationPrimitiveReader.Read_guid();
			}
			if (primitiveType == typeof(TimeSpan) && System.LocalAppContextSwitches.EnableTimeSpanSerialization)
			{
				return xmlSerializationPrimitiveReader.Read_TimeSpan();
			}
			throw new InvalidOperationException(Res.GetString("The type {0} was not expected. Use the XmlInclude or SoapInclude attribute to specify types that are not known statically.", primitiveType.FullName));
		}
	}
}
