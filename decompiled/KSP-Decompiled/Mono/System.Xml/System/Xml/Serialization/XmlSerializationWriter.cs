using System.Collections;
using System.Globalization;
using System.Reflection;
using System.Text;
using System.Xml.Schema;

namespace System.Xml.Serialization;

public abstract class XmlSerializationWriter : XmlSerializationGeneratedCode
{
	internal class TypeEntry
	{
		internal XmlSerializationWriteCallback callback;

		internal string typeNs;

		internal string typeName;

		internal Type type;
	}

	private XmlWriter w;

	private XmlSerializerNamespaces namespaces;

	private int tempNamespacePrefix;

	private Hashtable usedPrefixes;

	private Hashtable references;

	private string idBase;

	private int nextId;

	private Hashtable typeEntries;

	private ArrayList referencesToWrite;

	private Hashtable objectsInUse;

	private string aliasBase = "q";

	private bool soap12;

	private bool escapeName = true;

	protected bool EscapeName
	{
		get
		{
			return escapeName;
		}
		set
		{
			escapeName = value;
		}
	}

	protected XmlWriter Writer
	{
		get
		{
			return w;
		}
		set
		{
			w = value;
		}
	}

	protected ArrayList Namespaces
	{
		get
		{
			if (namespaces != null)
			{
				return namespaces.NamespaceList;
			}
			return null;
		}
		set
		{
			if (value == null)
			{
				namespaces = null;
				return;
			}
			XmlQualifiedName[] array = (XmlQualifiedName[])value.ToArray(typeof(XmlQualifiedName));
			namespaces = new XmlSerializerNamespaces(array);
		}
	}

	internal void Init(XmlWriter w, XmlSerializerNamespaces namespaces, string encodingStyle, string idBase, TempAssembly tempAssembly)
	{
		this.w = w;
		this.namespaces = namespaces;
		soap12 = encodingStyle == "http://www.w3.org/2003/05/soap-encoding";
		this.idBase = idBase;
		Init(tempAssembly);
	}

	protected static byte[] FromByteArrayBase64(byte[] value)
	{
		return value;
	}

	protected static Assembly ResolveDynamicAssembly(string assemblyFullName)
	{
		return DynamicAssemblies.Get(assemblyFullName);
	}

	protected static string FromByteArrayHex(byte[] value)
	{
		return XmlCustomFormatter.FromByteArrayHex(value);
	}

	protected static string FromDateTime(DateTime value)
	{
		return XmlCustomFormatter.FromDateTime(value);
	}

	protected static string FromDate(DateTime value)
	{
		return XmlCustomFormatter.FromDate(value);
	}

	protected static string FromTime(DateTime value)
	{
		return XmlCustomFormatter.FromTime(value);
	}

	protected static string FromChar(char value)
	{
		return XmlCustomFormatter.FromChar(value);
	}

	protected static string FromEnum(long value, string[] values, long[] ids)
	{
		return XmlCustomFormatter.FromEnum(value, values, ids, null);
	}

	protected static string FromEnum(long value, string[] values, long[] ids, string typeName)
	{
		return XmlCustomFormatter.FromEnum(value, values, ids, typeName);
	}

	protected static string FromXmlName(string name)
	{
		return XmlCustomFormatter.FromXmlName(name);
	}

	protected static string FromXmlNCName(string ncName)
	{
		return XmlCustomFormatter.FromXmlNCName(ncName);
	}

	protected static string FromXmlNmToken(string nmToken)
	{
		return XmlCustomFormatter.FromXmlNmToken(nmToken);
	}

	protected static string FromXmlNmTokens(string nmTokens)
	{
		return XmlCustomFormatter.FromXmlNmTokens(nmTokens);
	}

	protected void WriteXsiType(string name, string ns)
	{
		WriteAttribute("type", "http://www.w3.org/2001/XMLSchema-instance", GetQualifiedName(name, ns));
	}

	private XmlQualifiedName GetPrimitiveTypeName(Type type)
	{
		return GetPrimitiveTypeName(type, throwIfUnknown: true);
	}

	private XmlQualifiedName GetPrimitiveTypeName(Type type, bool throwIfUnknown)
	{
		XmlQualifiedName primitiveTypeNameInternal = GetPrimitiveTypeNameInternal(type);
		if (throwIfUnknown && primitiveTypeNameInternal == null)
		{
			throw CreateUnknownTypeException(type);
		}
		return primitiveTypeNameInternal;
	}

	internal static XmlQualifiedName GetPrimitiveTypeNameInternal(Type type)
	{
		string ns = "http://www.w3.org/2001/XMLSchema";
		string name;
		switch (Type.GetTypeCode(type))
		{
		case TypeCode.String:
			name = "string";
			break;
		case TypeCode.Int32:
			name = "int";
			break;
		case TypeCode.Boolean:
			name = "boolean";
			break;
		case TypeCode.Int16:
			name = "short";
			break;
		case TypeCode.Int64:
			name = "long";
			break;
		case TypeCode.Single:
			name = "float";
			break;
		case TypeCode.Double:
			name = "double";
			break;
		case TypeCode.Decimal:
			name = "decimal";
			break;
		case TypeCode.DateTime:
			name = "dateTime";
			break;
		case TypeCode.Byte:
			name = "unsignedByte";
			break;
		case TypeCode.SByte:
			name = "byte";
			break;
		case TypeCode.UInt16:
			name = "unsignedShort";
			break;
		case TypeCode.UInt32:
			name = "unsignedInt";
			break;
		case TypeCode.UInt64:
			name = "unsignedLong";
			break;
		case TypeCode.Char:
			name = "char";
			ns = "http://microsoft.com/wsdl/types/";
			break;
		default:
			if (type == typeof(XmlQualifiedName))
			{
				name = "QName";
				break;
			}
			if (type == typeof(byte[]))
			{
				name = "base64Binary";
				break;
			}
			if (type == typeof(TimeSpan) && System.LocalAppContextSwitches.EnableTimeSpanSerialization)
			{
				name = "TimeSpan";
				break;
			}
			if (type == typeof(Guid))
			{
				name = "guid";
				ns = "http://microsoft.com/wsdl/types/";
				break;
			}
			if (type == typeof(XmlNode[]))
			{
				name = "anyType";
				break;
			}
			return null;
		}
		return new XmlQualifiedName(name, ns);
	}

	protected void WriteTypedPrimitive(string name, string ns, object o, bool xsiType)
	{
		string text = null;
		string ns2 = "http://www.w3.org/2001/XMLSchema";
		bool flag = true;
		bool flag2 = false;
		Type type = o.GetType();
		bool flag3 = false;
		string text2;
		switch (Type.GetTypeCode(type))
		{
		case TypeCode.String:
			text = (string)o;
			text2 = "string";
			flag = false;
			break;
		case TypeCode.Int32:
			text = XmlConvert.ToString((int)o);
			text2 = "int";
			break;
		case TypeCode.Boolean:
			text = XmlConvert.ToString((bool)o);
			text2 = "boolean";
			break;
		case TypeCode.Int16:
			text = XmlConvert.ToString((short)o);
			text2 = "short";
			break;
		case TypeCode.Int64:
			text = XmlConvert.ToString((long)o);
			text2 = "long";
			break;
		case TypeCode.Single:
			text = XmlConvert.ToString((float)o);
			text2 = "float";
			break;
		case TypeCode.Double:
			text = XmlConvert.ToString((double)o);
			text2 = "double";
			break;
		case TypeCode.Decimal:
			text = XmlConvert.ToString((decimal)o);
			text2 = "decimal";
			break;
		case TypeCode.DateTime:
			text = FromDateTime((DateTime)o);
			text2 = "dateTime";
			break;
		case TypeCode.Char:
			text = FromChar((char)o);
			text2 = "char";
			ns2 = "http://microsoft.com/wsdl/types/";
			break;
		case TypeCode.Byte:
			text = XmlConvert.ToString((byte)o);
			text2 = "unsignedByte";
			break;
		case TypeCode.SByte:
			text = XmlConvert.ToString((sbyte)o);
			text2 = "byte";
			break;
		case TypeCode.UInt16:
			text = XmlConvert.ToString((ushort)o);
			text2 = "unsignedShort";
			break;
		case TypeCode.UInt32:
			text = XmlConvert.ToString((uint)o);
			text2 = "unsignedInt";
			break;
		case TypeCode.UInt64:
			text = XmlConvert.ToString((ulong)o);
			text2 = "unsignedLong";
			break;
		default:
			if (type == typeof(XmlQualifiedName))
			{
				text2 = "QName";
				flag3 = true;
				if (name == null)
				{
					w.WriteStartElement(text2, ns2);
				}
				else
				{
					w.WriteStartElement(name, ns);
				}
				text = FromXmlQualifiedName((XmlQualifiedName)o, ignoreEmpty: false);
				break;
			}
			if (type == typeof(byte[]))
			{
				text = string.Empty;
				flag2 = true;
				text2 = "base64Binary";
				break;
			}
			if (type == typeof(Guid))
			{
				text = XmlConvert.ToString((Guid)o);
				text2 = "guid";
				ns2 = "http://microsoft.com/wsdl/types/";
				break;
			}
			if (type == typeof(TimeSpan) && System.LocalAppContextSwitches.EnableTimeSpanSerialization)
			{
				text = XmlConvert.ToString((TimeSpan)o);
				text2 = "TimeSpan";
				break;
			}
			if (typeof(XmlNode[]).IsAssignableFrom(type))
			{
				if (name == null)
				{
					w.WriteStartElement("anyType", "http://www.w3.org/2001/XMLSchema");
				}
				else
				{
					w.WriteStartElement(name, ns);
				}
				XmlNode[] array = (XmlNode[])o;
				for (int i = 0; i < array.Length; i++)
				{
					if (array[i] != null)
					{
						array[i].WriteTo(w);
					}
				}
				w.WriteEndElement();
				return;
			}
			throw CreateUnknownTypeException(type);
		}
		if (!flag3)
		{
			if (name == null)
			{
				w.WriteStartElement(text2, ns2);
			}
			else
			{
				w.WriteStartElement(name, ns);
			}
		}
		if (xsiType)
		{
			WriteXsiType(text2, ns2);
		}
		if (text == null)
		{
			w.WriteAttributeString("nil", "http://www.w3.org/2001/XMLSchema-instance", "true");
		}
		else if (flag2)
		{
			XmlCustomFormatter.WriteArrayBase64(w, (byte[])o, 0, ((byte[])o).Length);
		}
		else if (flag)
		{
			w.WriteRaw(text);
		}
		else
		{
			w.WriteString(text);
		}
		w.WriteEndElement();
	}

	private string GetQualifiedName(string name, string ns)
	{
		if (ns == null || ns.Length == 0)
		{
			return name;
		}
		string text = w.LookupPrefix(ns);
		if (text == null)
		{
			if (ns == "http://www.w3.org/XML/1998/namespace")
			{
				text = "xml";
			}
			else
			{
				text = NextPrefix();
				WriteAttribute("xmlns", text, null, ns);
			}
		}
		else if (text.Length == 0)
		{
			return name;
		}
		return text + ":" + name;
	}

	protected string FromXmlQualifiedName(XmlQualifiedName xmlQualifiedName)
	{
		return FromXmlQualifiedName(xmlQualifiedName, ignoreEmpty: true);
	}

	protected string FromXmlQualifiedName(XmlQualifiedName xmlQualifiedName, bool ignoreEmpty)
	{
		if (xmlQualifiedName == null)
		{
			return null;
		}
		if (xmlQualifiedName.IsEmpty && ignoreEmpty)
		{
			return null;
		}
		return GetQualifiedName(EscapeName ? XmlConvert.EncodeLocalName(xmlQualifiedName.Name) : xmlQualifiedName.Name, xmlQualifiedName.Namespace);
	}

	protected void WriteStartElement(string name)
	{
		WriteStartElement(name, null, null, writePrefixed: false, null);
	}

	protected void WriteStartElement(string name, string ns)
	{
		WriteStartElement(name, ns, null, writePrefixed: false, null);
	}

	protected void WriteStartElement(string name, string ns, bool writePrefixed)
	{
		WriteStartElement(name, ns, null, writePrefixed, null);
	}

	protected void WriteStartElement(string name, string ns, object o)
	{
		WriteStartElement(name, ns, o, writePrefixed: false, null);
	}

	protected void WriteStartElement(string name, string ns, object o, bool writePrefixed)
	{
		WriteStartElement(name, ns, o, writePrefixed, null);
	}

	protected void WriteStartElement(string name, string ns, object o, bool writePrefixed, XmlSerializerNamespaces xmlns)
	{
		if (o != null && objectsInUse != null)
		{
			if (objectsInUse.ContainsKey(o))
			{
				throw new InvalidOperationException(Res.GetString("A circular reference was detected while serializing an object of type {0}.", o.GetType().FullName));
			}
			objectsInUse.Add(o, o);
		}
		string text = null;
		bool flag = false;
		if (namespaces != null)
		{
			foreach (string key in namespaces.Namespaces.Keys)
			{
				string text3 = (string)namespaces.Namespaces[key];
				if (key.Length > 0 && text3 == ns)
				{
					text = key;
				}
				if (key.Length == 0)
				{
					if (text3 == null || text3.Length == 0)
					{
						flag = true;
					}
					if (ns != text3)
					{
						writePrefixed = true;
					}
				}
			}
			usedPrefixes = ListUsedPrefixes(namespaces.Namespaces, aliasBase);
		}
		if (writePrefixed && text == null && ns != null && ns.Length > 0)
		{
			text = w.LookupPrefix(ns);
			if (text == null || text.Length == 0)
			{
				text = NextPrefix();
			}
		}
		if (text == null && xmlns != null)
		{
			text = xmlns.LookupPrefix(ns);
		}
		if (flag && text == null && ns != null && ns.Length != 0)
		{
			text = NextPrefix();
		}
		w.WriteStartElement(text, name, ns);
		if (namespaces != null)
		{
			foreach (string key2 in namespaces.Namespaces.Keys)
			{
				string text5 = (string)namespaces.Namespaces[key2];
				if (key2.Length == 0 && (text5 == null || text5.Length == 0))
				{
					continue;
				}
				if (text5 == null || text5.Length == 0)
				{
					if (key2.Length > 0)
					{
						throw new InvalidOperationException(Res.GetString("Invalid namespace attribute: xmlns:{0}=\"\".", key2));
					}
					WriteAttribute("xmlns", key2, null, text5);
				}
				else if (w.LookupPrefix(text5) == null)
				{
					if (text == null && key2.Length == 0)
					{
						break;
					}
					WriteAttribute("xmlns", key2, null, text5);
				}
			}
		}
		WriteNamespaceDeclarations(xmlns);
	}

	private Hashtable ListUsedPrefixes(Hashtable nsList, string prefix)
	{
		Hashtable hashtable = new Hashtable();
		int length = prefix.Length;
		foreach (string key in namespaces.Namespaces.Keys)
		{
			if (key.Length <= length)
			{
				continue;
			}
			string text2 = key;
			_ = text2.Length;
			if (text2.Length <= length || text2.Length > length + "2147483647".Length || !text2.StartsWith(prefix, StringComparison.Ordinal))
			{
				continue;
			}
			bool flag = true;
			for (int i = length; i < text2.Length; i++)
			{
				if (!char.IsDigit(text2, i))
				{
					flag = false;
					break;
				}
			}
			if (!flag)
			{
				continue;
			}
			long num = long.Parse(text2.Substring(length), CultureInfo.InvariantCulture);
			if (num <= int.MaxValue)
			{
				int num2 = (int)num;
				if (!hashtable.ContainsKey(num2))
				{
					hashtable.Add(num2, num2);
				}
			}
		}
		if (hashtable.Count > 0)
		{
			return hashtable;
		}
		return null;
	}

	protected void WriteNullTagEncoded(string name)
	{
		WriteNullTagEncoded(name, null);
	}

	protected void WriteNullTagEncoded(string name, string ns)
	{
		if (name != null && name.Length != 0)
		{
			WriteStartElement(name, ns, null, writePrefixed: true);
			w.WriteAttributeString("nil", "http://www.w3.org/2001/XMLSchema-instance", "true");
			w.WriteEndElement();
		}
	}

	protected void WriteNullTagLiteral(string name)
	{
		WriteNullTagLiteral(name, null);
	}

	protected void WriteNullTagLiteral(string name, string ns)
	{
		if (name != null && name.Length != 0)
		{
			WriteStartElement(name, ns, null, writePrefixed: false);
			w.WriteAttributeString("nil", "http://www.w3.org/2001/XMLSchema-instance", "true");
			w.WriteEndElement();
		}
	}

	protected void WriteEmptyTag(string name)
	{
		WriteEmptyTag(name, null);
	}

	protected void WriteEmptyTag(string name, string ns)
	{
		if (name != null && name.Length != 0)
		{
			WriteStartElement(name, ns, null, writePrefixed: false);
			w.WriteEndElement();
		}
	}

	protected void WriteEndElement()
	{
		w.WriteEndElement();
	}

	protected void WriteEndElement(object o)
	{
		w.WriteEndElement();
		if (o != null && objectsInUse != null)
		{
			objectsInUse.Remove(o);
		}
	}

	protected void WriteSerializable(IXmlSerializable serializable, string name, string ns, bool isNullable)
	{
		WriteSerializable(serializable, name, ns, isNullable, wrapped: true);
	}

	protected void WriteSerializable(IXmlSerializable serializable, string name, string ns, bool isNullable, bool wrapped)
	{
		if (serializable == null)
		{
			if (isNullable)
			{
				WriteNullTagLiteral(name, ns);
			}
			return;
		}
		if (wrapped)
		{
			w.WriteStartElement(name, ns);
		}
		serializable.WriteXml(w);
		if (wrapped)
		{
			w.WriteEndElement();
		}
	}

	protected void WriteNullableStringEncoded(string name, string ns, string value, XmlQualifiedName xsiType)
	{
		if (value == null)
		{
			WriteNullTagEncoded(name, ns);
		}
		else
		{
			WriteElementString(name, ns, value, xsiType);
		}
	}

	protected void WriteNullableStringLiteral(string name, string ns, string value)
	{
		if (value == null)
		{
			WriteNullTagLiteral(name, ns);
		}
		else
		{
			WriteElementString(name, ns, value, null);
		}
	}

	protected void WriteNullableStringEncodedRaw(string name, string ns, string value, XmlQualifiedName xsiType)
	{
		if (value == null)
		{
			WriteNullTagEncoded(name, ns);
		}
		else
		{
			WriteElementStringRaw(name, ns, value, xsiType);
		}
	}

	protected void WriteNullableStringEncodedRaw(string name, string ns, byte[] value, XmlQualifiedName xsiType)
	{
		if (value == null)
		{
			WriteNullTagEncoded(name, ns);
		}
		else
		{
			WriteElementStringRaw(name, ns, value, xsiType);
		}
	}

	protected void WriteNullableStringLiteralRaw(string name, string ns, string value)
	{
		if (value == null)
		{
			WriteNullTagLiteral(name, ns);
		}
		else
		{
			WriteElementStringRaw(name, ns, value, null);
		}
	}

	protected void WriteNullableStringLiteralRaw(string name, string ns, byte[] value)
	{
		if (value == null)
		{
			WriteNullTagLiteral(name, ns);
		}
		else
		{
			WriteElementStringRaw(name, ns, value, null);
		}
	}

	protected void WriteNullableQualifiedNameEncoded(string name, string ns, XmlQualifiedName value, XmlQualifiedName xsiType)
	{
		if (value == null)
		{
			WriteNullTagEncoded(name, ns);
		}
		else
		{
			WriteElementQualifiedName(name, ns, value, xsiType);
		}
	}

	protected void WriteNullableQualifiedNameLiteral(string name, string ns, XmlQualifiedName value)
	{
		if (value == null)
		{
			WriteNullTagLiteral(name, ns);
		}
		else
		{
			WriteElementQualifiedName(name, ns, value, null);
		}
	}

	protected void WriteElementEncoded(XmlNode node, string name, string ns, bool isNullable, bool any)
	{
		if (node == null)
		{
			if (isNullable)
			{
				WriteNullTagEncoded(name, ns);
			}
		}
		else
		{
			WriteElement(node, name, ns, isNullable, any);
		}
	}

	protected void WriteElementLiteral(XmlNode node, string name, string ns, bool isNullable, bool any)
	{
		if (node == null)
		{
			if (isNullable)
			{
				WriteNullTagLiteral(name, ns);
			}
		}
		else
		{
			WriteElement(node, name, ns, isNullable, any);
		}
	}

	private void WriteElement(XmlNode node, string name, string ns, bool isNullable, bool any)
	{
		if (typeof(XmlAttribute).IsAssignableFrom(node.GetType()))
		{
			throw new InvalidOperationException(Res.GetString("Cannot write a node of type XmlAttribute as an element value. Use XmlAnyAttributeAttribute with an array of XmlNode or XmlAttribute to write the node as an attribute."));
		}
		if (node is XmlDocument)
		{
			node = ((XmlDocument)node).DocumentElement;
			if (node == null)
			{
				if (isNullable)
				{
					WriteNullTagEncoded(name, ns);
				}
				return;
			}
		}
		if (any)
		{
			if (node is XmlElement && name != null && name.Length > 0 && (node.LocalName != name || node.NamespaceURI != ns))
			{
				throw new InvalidOperationException(Res.GetString("This element was named '{0}' from namespace '{1}' but should have been named '{2}' from namespace '{3}'.", node.LocalName, node.NamespaceURI, name, ns));
			}
		}
		else
		{
			w.WriteStartElement(name, ns);
		}
		node.WriteTo(w);
		if (!any)
		{
			w.WriteEndElement();
		}
	}

	protected Exception CreateUnknownTypeException(object o)
	{
		return CreateUnknownTypeException(o.GetType());
	}

	protected Exception CreateUnknownTypeException(Type type)
	{
		if (typeof(IXmlSerializable).IsAssignableFrom(type))
		{
			return new InvalidOperationException(Res.GetString("The type {0} may not be used in this context. To use {0} as a parameter, return type, or member of a class or struct, the parameter, return type, or member must be declared as type {0} (it cannot be object). Objects of type {0} may not be used in un-typed collections, such as ArrayLists.", type.FullName));
		}
		if (!new TypeScope().GetTypeDesc(type).IsStructLike)
		{
			return new InvalidOperationException(Res.GetString("The type {0} may not be used in this context.", type.FullName));
		}
		return new InvalidOperationException(Res.GetString("The type {0} was not expected. Use the XmlInclude or SoapInclude attribute to specify types that are not known statically.", type.FullName));
	}

	protected Exception CreateMismatchChoiceException(string value, string elementName, string enumValue)
	{
		return new InvalidOperationException(Res.GetString("Value of {0} mismatches the type of {1}; you need to set it to {2}.", elementName, value, enumValue));
	}

	protected Exception CreateUnknownAnyElementException(string name, string ns)
	{
		return new InvalidOperationException(Res.GetString("The XML element '{0}' from namespace '{1}' was not expected. The XML element name and namespace must match those provided via XmlAnyElementAttribute(s).", name, ns));
	}

	protected Exception CreateInvalidChoiceIdentifierValueException(string type, string identifier)
	{
		return new InvalidOperationException(Res.GetString("Invalid or missing value of the choice identifier '{1}' of type '{0}[]'.", type, identifier));
	}

	protected Exception CreateChoiceIdentifierValueException(string value, string identifier, string name, string ns)
	{
		return new InvalidOperationException(Res.GetString("Value '{0}' of the choice identifier '{1}' does not match element '{2}' from namespace '{3}'.", value, identifier, name, ns));
	}

	protected Exception CreateInvalidEnumValueException(object value, string typeName)
	{
		return new InvalidOperationException(Res.GetString("Instance validation error: '{0}' is not a valid value for {1}.", value, typeName));
	}

	protected Exception CreateInvalidAnyTypeException(object o)
	{
		return CreateInvalidAnyTypeException(o.GetType());
	}

	protected Exception CreateInvalidAnyTypeException(Type type)
	{
		return new InvalidOperationException(Res.GetString("Cannot serialize member of type {0}: XmlAnyElement can only be used with classes of type XmlNode or a type deriving from XmlNode.", type.FullName));
	}

	protected void WriteReferencingElement(string n, string ns, object o)
	{
		WriteReferencingElement(n, ns, o, isNullable: false);
	}

	protected void WriteReferencingElement(string n, string ns, object o, bool isNullable)
	{
		if (o == null)
		{
			if (isNullable)
			{
				WriteNullTagEncoded(n, ns);
			}
			return;
		}
		WriteStartElement(n, ns, null, writePrefixed: true);
		if (soap12)
		{
			w.WriteAttributeString("ref", "http://www.w3.org/2003/05/soap-encoding", GetId(o, addToReferencesList: true));
		}
		else
		{
			w.WriteAttributeString("href", "#" + GetId(o, addToReferencesList: true));
		}
		w.WriteEndElement();
	}

	private bool IsIdDefined(object o)
	{
		if (references != null)
		{
			return references.Contains(o);
		}
		return false;
	}

	private string GetId(object o, bool addToReferencesList)
	{
		if (references == null)
		{
			references = new Hashtable();
			referencesToWrite = new ArrayList();
		}
		string text = (string)references[o];
		if (text == null)
		{
			string text2 = idBase;
			int num = ++nextId;
			text = text2 + "id" + num.ToString(CultureInfo.InvariantCulture);
			references.Add(o, text);
			if (addToReferencesList)
			{
				referencesToWrite.Add(o);
			}
		}
		return text;
	}

	protected void WriteId(object o)
	{
		WriteId(o, addToReferencesList: true);
	}

	private void WriteId(object o, bool addToReferencesList)
	{
		if (soap12)
		{
			w.WriteAttributeString("id", "http://www.w3.org/2003/05/soap-encoding", GetId(o, addToReferencesList));
		}
		else
		{
			w.WriteAttributeString("id", GetId(o, addToReferencesList));
		}
	}

	protected void WriteXmlAttribute(XmlNode node)
	{
		WriteXmlAttribute(node, null);
	}

	protected void WriteXmlAttribute(XmlNode node, object container)
	{
		if (!(node is XmlAttribute xmlAttribute))
		{
			throw new InvalidOperationException(Res.GetString("The node must be either type XmlAttribute or a derived type."));
		}
		if (xmlAttribute.Value != null)
		{
			if (xmlAttribute.NamespaceURI == "http://schemas.xmlsoap.org/wsdl/" && xmlAttribute.LocalName == "arrayType")
			{
				string dims;
				XmlQualifiedName xmlQualifiedName = TypeScope.ParseWsdlArrayType(xmlAttribute.Value, out dims, (container is XmlSchemaObject) ? ((XmlSchemaObject)container) : null);
				string value = FromXmlQualifiedName(xmlQualifiedName, ignoreEmpty: true) + dims;
				WriteAttribute("arrayType", "http://schemas.xmlsoap.org/wsdl/", value);
			}
			else
			{
				WriteAttribute(xmlAttribute.Name, xmlAttribute.NamespaceURI, xmlAttribute.Value);
			}
		}
	}

	protected void WriteAttribute(string localName, string ns, string value)
	{
		if (value == null || localName == "xmlns" || localName.StartsWith("xmlns:", StringComparison.Ordinal))
		{
			return;
		}
		int num = localName.IndexOf(':');
		if (num < 0)
		{
			if (ns == "http://www.w3.org/XML/1998/namespace")
			{
				string text = w.LookupPrefix(ns);
				if (text == null || text.Length == 0)
				{
					text = "xml";
				}
				w.WriteAttributeString(text, localName, ns, value);
			}
			else
			{
				w.WriteAttributeString(localName, ns, value);
			}
		}
		else
		{
			string prefix = localName.Substring(0, num);
			w.WriteAttributeString(prefix, localName.Substring(num + 1), ns, value);
		}
	}

	protected void WriteAttribute(string localName, string ns, byte[] value)
	{
		if (value == null || localName == "xmlns" || localName.StartsWith("xmlns:", StringComparison.Ordinal))
		{
			return;
		}
		int num = localName.IndexOf(':');
		if (num < 0)
		{
			if (ns == "http://www.w3.org/XML/1998/namespace")
			{
				string text = w.LookupPrefix(ns);
				if (text == null || text.Length == 0)
				{
					text = "xml";
				}
				w.WriteStartAttribute("xml", localName, ns);
			}
			else
			{
				w.WriteStartAttribute(null, localName, ns);
			}
		}
		else
		{
			string text2 = localName.Substring(0, num);
			text2 = w.LookupPrefix(ns);
			w.WriteStartAttribute(text2, localName.Substring(num + 1), ns);
		}
		XmlCustomFormatter.WriteArrayBase64(w, value, 0, value.Length);
		w.WriteEndAttribute();
	}

	protected void WriteAttribute(string localName, string value)
	{
		if (value != null)
		{
			w.WriteAttributeString(localName, null, value);
		}
	}

	protected void WriteAttribute(string localName, byte[] value)
	{
		if (value != null)
		{
			w.WriteStartAttribute(null, localName, null);
			XmlCustomFormatter.WriteArrayBase64(w, value, 0, value.Length);
			w.WriteEndAttribute();
		}
	}

	protected void WriteAttribute(string prefix, string localName, string ns, string value)
	{
		if (value != null)
		{
			w.WriteAttributeString(prefix, localName, null, value);
		}
	}

	protected void WriteValue(string value)
	{
		if (value != null)
		{
			w.WriteString(value);
		}
	}

	protected void WriteValue(byte[] value)
	{
		if (value != null)
		{
			XmlCustomFormatter.WriteArrayBase64(w, value, 0, value.Length);
		}
	}

	protected void WriteStartDocument()
	{
		if (w.WriteState == WriteState.Start)
		{
			w.WriteStartDocument();
		}
	}

	protected void WriteElementString(string localName, string value)
	{
		WriteElementString(localName, null, value, null);
	}

	protected void WriteElementString(string localName, string ns, string value)
	{
		WriteElementString(localName, ns, value, null);
	}

	protected void WriteElementString(string localName, string value, XmlQualifiedName xsiType)
	{
		WriteElementString(localName, null, value, xsiType);
	}

	protected void WriteElementString(string localName, string ns, string value, XmlQualifiedName xsiType)
	{
		if (value != null)
		{
			if (xsiType == null)
			{
				w.WriteElementString(localName, ns, value);
				return;
			}
			w.WriteStartElement(localName, ns);
			WriteXsiType(xsiType.Name, xsiType.Namespace);
			w.WriteString(value);
			w.WriteEndElement();
		}
	}

	protected void WriteElementStringRaw(string localName, string value)
	{
		WriteElementStringRaw(localName, null, value, null);
	}

	protected void WriteElementStringRaw(string localName, byte[] value)
	{
		WriteElementStringRaw(localName, null, value, null);
	}

	protected void WriteElementStringRaw(string localName, string ns, string value)
	{
		WriteElementStringRaw(localName, ns, value, null);
	}

	protected void WriteElementStringRaw(string localName, string ns, byte[] value)
	{
		WriteElementStringRaw(localName, ns, value, null);
	}

	protected void WriteElementStringRaw(string localName, string value, XmlQualifiedName xsiType)
	{
		WriteElementStringRaw(localName, null, value, xsiType);
	}

	protected void WriteElementStringRaw(string localName, byte[] value, XmlQualifiedName xsiType)
	{
		WriteElementStringRaw(localName, null, value, xsiType);
	}

	protected void WriteElementStringRaw(string localName, string ns, string value, XmlQualifiedName xsiType)
	{
		if (value != null)
		{
			w.WriteStartElement(localName, ns);
			if (xsiType != null)
			{
				WriteXsiType(xsiType.Name, xsiType.Namespace);
			}
			w.WriteRaw(value);
			w.WriteEndElement();
		}
	}

	protected void WriteElementStringRaw(string localName, string ns, byte[] value, XmlQualifiedName xsiType)
	{
		if (value != null)
		{
			w.WriteStartElement(localName, ns);
			if (xsiType != null)
			{
				WriteXsiType(xsiType.Name, xsiType.Namespace);
			}
			XmlCustomFormatter.WriteArrayBase64(w, value, 0, value.Length);
			w.WriteEndElement();
		}
	}

	protected void WriteRpcResult(string name, string ns)
	{
		if (soap12)
		{
			WriteElementQualifiedName("result", "http://www.w3.org/2003/05/soap-rpc", new XmlQualifiedName(name, ns), null);
		}
	}

	protected void WriteElementQualifiedName(string localName, XmlQualifiedName value)
	{
		WriteElementQualifiedName(localName, null, value, null);
	}

	protected void WriteElementQualifiedName(string localName, XmlQualifiedName value, XmlQualifiedName xsiType)
	{
		WriteElementQualifiedName(localName, null, value, xsiType);
	}

	protected void WriteElementQualifiedName(string localName, string ns, XmlQualifiedName value)
	{
		WriteElementQualifiedName(localName, ns, value, null);
	}

	protected void WriteElementQualifiedName(string localName, string ns, XmlQualifiedName value, XmlQualifiedName xsiType)
	{
		if (!(value == null))
		{
			if (value.Namespace == null || value.Namespace.Length == 0)
			{
				WriteStartElement(localName, ns, null, writePrefixed: true);
				WriteAttribute("xmlns", "");
			}
			else
			{
				w.WriteStartElement(localName, ns);
			}
			if (xsiType != null)
			{
				WriteXsiType(xsiType.Name, xsiType.Namespace);
			}
			w.WriteString(FromXmlQualifiedName(value, ignoreEmpty: false));
			w.WriteEndElement();
		}
	}

	protected void AddWriteCallback(Type type, string typeName, string typeNs, XmlSerializationWriteCallback callback)
	{
		TypeEntry typeEntry = new TypeEntry();
		typeEntry.typeName = typeName;
		typeEntry.typeNs = typeNs;
		typeEntry.type = type;
		typeEntry.callback = callback;
		typeEntries[type] = typeEntry;
	}

	private void WriteArray(string name, string ns, object o, Type type)
	{
		Type arrayElementType = TypeScope.GetArrayElementType(type, null);
		StringBuilder stringBuilder = new StringBuilder();
		if (!soap12)
		{
			while ((arrayElementType.IsArray || typeof(IEnumerable).IsAssignableFrom(arrayElementType)) && GetPrimitiveTypeName(arrayElementType, throwIfUnknown: false) == null)
			{
				arrayElementType = TypeScope.GetArrayElementType(arrayElementType, null);
				stringBuilder.Append("[]");
			}
		}
		string text;
		string ns2;
		if (arrayElementType == typeof(object))
		{
			text = "anyType";
			ns2 = "http://www.w3.org/2001/XMLSchema";
		}
		else
		{
			TypeEntry typeEntry = GetTypeEntry(arrayElementType);
			if (typeEntry != null)
			{
				text = typeEntry.typeName;
				ns2 = typeEntry.typeNs;
			}
			else if (soap12)
			{
				XmlQualifiedName primitiveTypeName = GetPrimitiveTypeName(arrayElementType, throwIfUnknown: false);
				if (primitiveTypeName != null)
				{
					text = primitiveTypeName.Name;
					ns2 = primitiveTypeName.Namespace;
				}
				else
				{
					Type baseType = arrayElementType.BaseType;
					while (baseType != null)
					{
						typeEntry = GetTypeEntry(baseType);
						if (typeEntry != null)
						{
							break;
						}
						baseType = baseType.BaseType;
					}
					if (typeEntry != null)
					{
						text = typeEntry.typeName;
						ns2 = typeEntry.typeNs;
					}
					else
					{
						text = "anyType";
						ns2 = "http://www.w3.org/2001/XMLSchema";
					}
				}
			}
			else
			{
				XmlQualifiedName primitiveTypeName2 = GetPrimitiveTypeName(arrayElementType);
				text = primitiveTypeName2.Name;
				ns2 = primitiveTypeName2.Namespace;
			}
		}
		if (stringBuilder.Length > 0)
		{
			text += stringBuilder.ToString();
		}
		if (soap12 && name != null && name.Length > 0)
		{
			WriteStartElement(name, ns, null, writePrefixed: false);
		}
		else
		{
			WriteStartElement("Array", "http://schemas.xmlsoap.org/soap/encoding/", null, writePrefixed: true);
		}
		WriteId(o, addToReferencesList: false);
		if (type.IsArray)
		{
			Array array = (Array)o;
			int length = array.Length;
			if (soap12)
			{
				w.WriteAttributeString("itemType", "http://www.w3.org/2003/05/soap-encoding", GetQualifiedName(text, ns2));
				w.WriteAttributeString("arraySize", "http://www.w3.org/2003/05/soap-encoding", length.ToString(CultureInfo.InvariantCulture));
			}
			else
			{
				w.WriteAttributeString("arrayType", "http://schemas.xmlsoap.org/soap/encoding/", GetQualifiedName(text, ns2) + "[" + length.ToString(CultureInfo.InvariantCulture) + "]");
			}
			for (int i = 0; i < length; i++)
			{
				WritePotentiallyReferencingElement("Item", "", array.GetValue(i), arrayElementType, suppressReference: false, isNullable: true);
			}
		}
		else
		{
			int num = (typeof(ICollection).IsAssignableFrom(type) ? ((ICollection)o).Count : (-1));
			if (soap12)
			{
				w.WriteAttributeString("itemType", "http://www.w3.org/2003/05/soap-encoding", GetQualifiedName(text, ns2));
				if (num >= 0)
				{
					w.WriteAttributeString("arraySize", "http://www.w3.org/2003/05/soap-encoding", num.ToString(CultureInfo.InvariantCulture));
				}
			}
			else
			{
				string text2 = ((num >= 0) ? ("[" + num + "]") : "[]");
				w.WriteAttributeString("arrayType", "http://schemas.xmlsoap.org/soap/encoding/", GetQualifiedName(text, ns2) + text2);
			}
			IEnumerator enumerator = ((IEnumerable)o).GetEnumerator();
			if (enumerator != null)
			{
				while (enumerator.MoveNext())
				{
					WritePotentiallyReferencingElement("Item", "", enumerator.Current, arrayElementType, suppressReference: false, isNullable: true);
				}
			}
		}
		w.WriteEndElement();
	}

	protected void WritePotentiallyReferencingElement(string n, string ns, object o)
	{
		WritePotentiallyReferencingElement(n, ns, o, null, suppressReference: false, isNullable: false);
	}

	protected void WritePotentiallyReferencingElement(string n, string ns, object o, Type ambientType)
	{
		WritePotentiallyReferencingElement(n, ns, o, ambientType, suppressReference: false, isNullable: false);
	}

	protected void WritePotentiallyReferencingElement(string n, string ns, object o, Type ambientType, bool suppressReference)
	{
		WritePotentiallyReferencingElement(n, ns, o, ambientType, suppressReference, isNullable: false);
	}

	protected void WritePotentiallyReferencingElement(string n, string ns, object o, Type ambientType, bool suppressReference, bool isNullable)
	{
		if (o == null)
		{
			if (isNullable)
			{
				WriteNullTagEncoded(n, ns);
			}
			return;
		}
		Type type = o.GetType();
		if (Convert.GetTypeCode(o) == TypeCode.Object && !(o is Guid) && type != typeof(XmlQualifiedName) && !(o is XmlNode[]) && type != typeof(byte[]))
		{
			if ((suppressReference || soap12) && !IsIdDefined(o))
			{
				WriteReferencedElement(n, ns, o, ambientType);
			}
			else if (n == null)
			{
				TypeEntry typeEntry = GetTypeEntry(type);
				WriteReferencingElement(typeEntry.typeName, typeEntry.typeNs, o, isNullable);
			}
			else
			{
				WriteReferencingElement(n, ns, o, isNullable);
			}
			return;
		}
		bool flag = type != ambientType && !type.IsEnum;
		TypeEntry typeEntry2 = GetTypeEntry(type);
		if (typeEntry2 != null)
		{
			if (n == null)
			{
				WriteStartElement(typeEntry2.typeName, typeEntry2.typeNs, null, writePrefixed: true);
			}
			else
			{
				WriteStartElement(n, ns, null, writePrefixed: true);
			}
			if (flag)
			{
				WriteXsiType(typeEntry2.typeName, typeEntry2.typeNs);
			}
			typeEntry2.callback(o);
			w.WriteEndElement();
		}
		else
		{
			WriteTypedPrimitive(n, ns, o, flag);
		}
	}

	private void WriteReferencedElement(object o, Type ambientType)
	{
		WriteReferencedElement(null, null, o, ambientType);
	}

	private void WriteReferencedElement(string name, string ns, object o, Type ambientType)
	{
		if (name == null)
		{
			name = string.Empty;
		}
		Type type = o.GetType();
		if (type.IsArray || typeof(IEnumerable).IsAssignableFrom(type))
		{
			WriteArray(name, ns, o, type);
			return;
		}
		TypeEntry typeEntry = GetTypeEntry(type);
		if (typeEntry == null)
		{
			throw CreateUnknownTypeException(type);
		}
		WriteStartElement((name.Length == 0) ? typeEntry.typeName : name, (ns == null) ? typeEntry.typeNs : ns, null, writePrefixed: true);
		WriteId(o, addToReferencesList: false);
		if (ambientType != type)
		{
			WriteXsiType(typeEntry.typeName, typeEntry.typeNs);
		}
		typeEntry.callback(o);
		w.WriteEndElement();
	}

	private TypeEntry GetTypeEntry(Type t)
	{
		if (typeEntries == null)
		{
			typeEntries = new Hashtable();
			InitCallbacks();
		}
		return (TypeEntry)typeEntries[t];
	}

	protected abstract void InitCallbacks();

	protected void WriteReferencedElements()
	{
		if (referencesToWrite != null)
		{
			for (int i = 0; i < referencesToWrite.Count; i++)
			{
				WriteReferencedElement(referencesToWrite[i], null);
			}
		}
	}

	protected void TopLevelElement()
	{
		objectsInUse = new Hashtable();
	}

	protected void WriteNamespaceDeclarations(XmlSerializerNamespaces xmlns)
	{
		if (xmlns != null)
		{
			foreach (DictionaryEntry @namespace in xmlns.Namespaces)
			{
				string text = (string)@namespace.Key;
				string text2 = (string)@namespace.Value;
				if (namespaces != null && namespaces.Namespaces[text] is string text3 && text3 != text2)
				{
					throw new InvalidOperationException(Res.GetString("Illegal namespace declaration xmlns:{0}='{1}'. Namespace alias '{0}' already defined in the current scope.", text, text2));
				}
				string text4 = ((text2 == null || text2.Length == 0) ? null : Writer.LookupPrefix(text2));
				if (text4 == null || text4 != text)
				{
					WriteAttribute("xmlns", text, null, text2);
				}
			}
		}
		namespaces = null;
	}

	private string NextPrefix()
	{
		if (usedPrefixes == null)
		{
			return aliasBase + ++tempNamespacePrefix;
		}
		while (usedPrefixes.ContainsKey(++tempNamespacePrefix))
		{
		}
		return aliasBase + tempNamespacePrefix;
	}
}
