using System.Collections;
using System.Configuration;
using System.Globalization;
using System.Reflection;
using System.Threading;
using System.Xml.Serialization.Configuration;

namespace System.Xml.Serialization;

public abstract class XmlSerializationReader : XmlSerializationGeneratedCode
{
	private struct SoapArrayInfo
	{
		public string qname;

		public int dimensions;

		public int length;

		public int jaggedDimensions;
	}

	protected class Fixup
	{
		private XmlSerializationFixupCallback callback;

		private object source;

		private string[] ids;

		public XmlSerializationFixupCallback Callback => callback;

		public object Source
		{
			get
			{
				return source;
			}
			set
			{
				source = value;
			}
		}

		public string[] Ids => ids;

		public Fixup(object o, XmlSerializationFixupCallback callback, int count)
			: this(o, callback, new string[count])
		{
		}

		public Fixup(object o, XmlSerializationFixupCallback callback, string[] ids)
		{
			this.callback = callback;
			Source = o;
			this.ids = ids;
		}
	}

	protected class CollectionFixup
	{
		private XmlSerializationCollectionFixupCallback callback;

		private object collection;

		private object collectionItems;

		public XmlSerializationCollectionFixupCallback Callback => callback;

		public object Collection => collection;

		public object CollectionItems => collectionItems;

		public CollectionFixup(object collection, XmlSerializationCollectionFixupCallback callback, object collectionItems)
		{
			this.callback = callback;
			this.collection = collection;
			this.collectionItems = collectionItems;
		}
	}

	private XmlReader r;

	private XmlCountingReader countingReader;

	private XmlDocument d;

	private Hashtable callbacks;

	private Hashtable types;

	private Hashtable typesReverse;

	private XmlDeserializationEvents events;

	private Hashtable targets;

	private Hashtable referencedTargets;

	private ArrayList targetsWithoutIds;

	private ArrayList fixups;

	private ArrayList collectionFixups;

	private bool soap12;

	private bool isReturnValue;

	private bool decodeName = true;

	private string schemaNsID;

	private string schemaNs1999ID;

	private string schemaNs2000ID;

	private string schemaNonXsdTypesNsID;

	private string instanceNsID;

	private string instanceNs2000ID;

	private string instanceNs1999ID;

	private string soapNsID;

	private string soap12NsID;

	private string schemaID;

	private string wsdlNsID;

	private string wsdlArrayTypeID;

	private string nullID;

	private string nilID;

	private string typeID;

	private string arrayTypeID;

	private string itemTypeID;

	private string arraySizeID;

	private string arrayID;

	private string urTypeID;

	private string stringID;

	private string intID;

	private string booleanID;

	private string shortID;

	private string longID;

	private string floatID;

	private string doubleID;

	private string decimalID;

	private string dateTimeID;

	private string qnameID;

	private string dateID;

	private string timeID;

	private string hexBinaryID;

	private string base64BinaryID;

	private string base64ID;

	private string unsignedByteID;

	private string byteID;

	private string unsignedShortID;

	private string unsignedIntID;

	private string unsignedLongID;

	private string oldDecimalID;

	private string oldTimeInstantID;

	private string anyURIID;

	private string durationID;

	private string ENTITYID;

	private string ENTITIESID;

	private string gDayID;

	private string gMonthID;

	private string gMonthDayID;

	private string gYearID;

	private string gYearMonthID;

	private string IDID;

	private string IDREFID;

	private string IDREFSID;

	private string integerID;

	private string languageID;

	private string NameID;

	private string NCNameID;

	private string NMTOKENID;

	private string NMTOKENSID;

	private string negativeIntegerID;

	private string nonPositiveIntegerID;

	private string nonNegativeIntegerID;

	private string normalizedStringID;

	private string NOTATIONID;

	private string positiveIntegerID;

	private string tokenID;

	private string charID;

	private string guidID;

	private string timeSpanID;

	private static bool checkDeserializeAdvances;

	protected bool DecodeName
	{
		get
		{
			return decodeName;
		}
		set
		{
			decodeName = value;
		}
	}

	protected XmlReader Reader => r;

	protected int ReaderCount
	{
		get
		{
			if (!checkDeserializeAdvances)
			{
				return 0;
			}
			return countingReader.AdvanceCount;
		}
	}

	protected XmlDocument Document
	{
		get
		{
			if (d == null)
			{
				d = new XmlDocument(r.NameTable);
				d.SetBaseURI(r.BaseURI);
			}
			return d;
		}
	}

	protected bool IsReturnValue
	{
		get
		{
			if (isReturnValue)
			{
				return !soap12;
			}
			return false;
		}
		set
		{
			isReturnValue = value;
		}
	}

	static XmlSerializationReader()
	{
		checkDeserializeAdvances = ConfigurationManager.GetSection(ConfigurationStrings.XmlSerializerSectionPath) is XmlSerializerSection xmlSerializerSection && xmlSerializerSection.CheckDeserializeAdvances;
	}

	protected abstract void InitIDs();

	internal void Init(XmlReader r, XmlDeserializationEvents events, string encodingStyle, TempAssembly tempAssembly)
	{
		this.events = events;
		if (checkDeserializeAdvances)
		{
			countingReader = new XmlCountingReader(r);
			this.r = countingReader;
		}
		else
		{
			this.r = r;
		}
		d = null;
		soap12 = encodingStyle == "http://www.w3.org/2003/05/soap-encoding";
		Init(tempAssembly);
		schemaNsID = r.NameTable.Add("http://www.w3.org/2001/XMLSchema");
		schemaNs2000ID = r.NameTable.Add("http://www.w3.org/2000/10/XMLSchema");
		schemaNs1999ID = r.NameTable.Add("http://www.w3.org/1999/XMLSchema");
		schemaNonXsdTypesNsID = r.NameTable.Add("http://microsoft.com/wsdl/types/");
		instanceNsID = r.NameTable.Add("http://www.w3.org/2001/XMLSchema-instance");
		instanceNs2000ID = r.NameTable.Add("http://www.w3.org/2000/10/XMLSchema-instance");
		instanceNs1999ID = r.NameTable.Add("http://www.w3.org/1999/XMLSchema-instance");
		soapNsID = r.NameTable.Add("http://schemas.xmlsoap.org/soap/encoding/");
		soap12NsID = r.NameTable.Add("http://www.w3.org/2003/05/soap-encoding");
		schemaID = r.NameTable.Add("schema");
		wsdlNsID = r.NameTable.Add("http://schemas.xmlsoap.org/wsdl/");
		wsdlArrayTypeID = r.NameTable.Add("arrayType");
		nullID = r.NameTable.Add("null");
		nilID = r.NameTable.Add("nil");
		typeID = r.NameTable.Add("type");
		arrayTypeID = r.NameTable.Add("arrayType");
		itemTypeID = r.NameTable.Add("itemType");
		arraySizeID = r.NameTable.Add("arraySize");
		arrayID = r.NameTable.Add("Array");
		urTypeID = r.NameTable.Add("anyType");
		InitIDs();
	}

	protected static Assembly ResolveDynamicAssembly(string assemblyFullName)
	{
		return DynamicAssemblies.Get(assemblyFullName);
	}

	private void InitPrimitiveIDs()
	{
		if (tokenID == null)
		{
			r.NameTable.Add("http://www.w3.org/2001/XMLSchema");
			r.NameTable.Add("http://microsoft.com/wsdl/types/");
			stringID = r.NameTable.Add("string");
			intID = r.NameTable.Add("int");
			booleanID = r.NameTable.Add("boolean");
			shortID = r.NameTable.Add("short");
			longID = r.NameTable.Add("long");
			floatID = r.NameTable.Add("float");
			doubleID = r.NameTable.Add("double");
			decimalID = r.NameTable.Add("decimal");
			dateTimeID = r.NameTable.Add("dateTime");
			qnameID = r.NameTable.Add("QName");
			dateID = r.NameTable.Add("date");
			timeID = r.NameTable.Add("time");
			hexBinaryID = r.NameTable.Add("hexBinary");
			base64BinaryID = r.NameTable.Add("base64Binary");
			unsignedByteID = r.NameTable.Add("unsignedByte");
			byteID = r.NameTable.Add("byte");
			unsignedShortID = r.NameTable.Add("unsignedShort");
			unsignedIntID = r.NameTable.Add("unsignedInt");
			unsignedLongID = r.NameTable.Add("unsignedLong");
			oldDecimalID = r.NameTable.Add("decimal");
			oldTimeInstantID = r.NameTable.Add("timeInstant");
			charID = r.NameTable.Add("char");
			guidID = r.NameTable.Add("guid");
			if (System.LocalAppContextSwitches.EnableTimeSpanSerialization)
			{
				timeSpanID = r.NameTable.Add("TimeSpan");
			}
			base64ID = r.NameTable.Add("base64");
			anyURIID = r.NameTable.Add("anyURI");
			durationID = r.NameTable.Add("duration");
			ENTITYID = r.NameTable.Add("ENTITY");
			ENTITIESID = r.NameTable.Add("ENTITIES");
			gDayID = r.NameTable.Add("gDay");
			gMonthID = r.NameTable.Add("gMonth");
			gMonthDayID = r.NameTable.Add("gMonthDay");
			gYearID = r.NameTable.Add("gYear");
			gYearMonthID = r.NameTable.Add("gYearMonth");
			IDID = r.NameTable.Add("ID");
			IDREFID = r.NameTable.Add("IDREF");
			IDREFSID = r.NameTable.Add("IDREFS");
			integerID = r.NameTable.Add("integer");
			languageID = r.NameTable.Add("language");
			NameID = r.NameTable.Add("Name");
			NCNameID = r.NameTable.Add("NCName");
			NMTOKENID = r.NameTable.Add("NMTOKEN");
			NMTOKENSID = r.NameTable.Add("NMTOKENS");
			negativeIntegerID = r.NameTable.Add("negativeInteger");
			nonNegativeIntegerID = r.NameTable.Add("nonNegativeInteger");
			nonPositiveIntegerID = r.NameTable.Add("nonPositiveInteger");
			normalizedStringID = r.NameTable.Add("normalizedString");
			NOTATIONID = r.NameTable.Add("NOTATION");
			positiveIntegerID = r.NameTable.Add("positiveInteger");
			tokenID = r.NameTable.Add("token");
		}
	}

	protected XmlQualifiedName GetXsiType()
	{
		string attribute = r.GetAttribute(typeID, instanceNsID);
		if (attribute == null)
		{
			attribute = r.GetAttribute(typeID, instanceNs2000ID);
			if (attribute == null)
			{
				attribute = r.GetAttribute(typeID, instanceNs1999ID);
				if (attribute == null)
				{
					return null;
				}
			}
		}
		return ToXmlQualifiedName(attribute, decodeName: false);
	}

	private Type GetPrimitiveType(XmlQualifiedName typeName, bool throwOnUnknown)
	{
		InitPrimitiveIDs();
		if ((object)typeName.Namespace == schemaNsID || (object)typeName.Namespace == soapNsID || (object)typeName.Namespace == soap12NsID)
		{
			if ((object)typeName.Name == stringID || (object)typeName.Name == anyURIID || (object)typeName.Name == durationID || (object)typeName.Name == ENTITYID || (object)typeName.Name == ENTITIESID || (object)typeName.Name == gDayID || (object)typeName.Name == gMonthID || (object)typeName.Name == gMonthDayID || (object)typeName.Name == gYearID || (object)typeName.Name == gYearMonthID || (object)typeName.Name == IDID || (object)typeName.Name == IDREFID || (object)typeName.Name == IDREFSID || (object)typeName.Name == integerID || (object)typeName.Name == languageID || (object)typeName.Name == NameID || (object)typeName.Name == NCNameID || (object)typeName.Name == NMTOKENID || (object)typeName.Name == NMTOKENSID || (object)typeName.Name == negativeIntegerID || (object)typeName.Name == nonPositiveIntegerID || (object)typeName.Name == nonNegativeIntegerID || (object)typeName.Name == normalizedStringID || (object)typeName.Name == NOTATIONID || (object)typeName.Name == positiveIntegerID || (object)typeName.Name == tokenID)
			{
				return typeof(string);
			}
			if ((object)typeName.Name == intID)
			{
				return typeof(int);
			}
			if ((object)typeName.Name == booleanID)
			{
				return typeof(bool);
			}
			if ((object)typeName.Name == shortID)
			{
				return typeof(short);
			}
			if ((object)typeName.Name == longID)
			{
				return typeof(long);
			}
			if ((object)typeName.Name == floatID)
			{
				return typeof(float);
			}
			if ((object)typeName.Name == doubleID)
			{
				return typeof(double);
			}
			if ((object)typeName.Name == decimalID)
			{
				return typeof(decimal);
			}
			if ((object)typeName.Name == dateTimeID)
			{
				return typeof(DateTime);
			}
			if ((object)typeName.Name == qnameID)
			{
				return typeof(XmlQualifiedName);
			}
			if ((object)typeName.Name == dateID)
			{
				return typeof(DateTime);
			}
			if ((object)typeName.Name == timeID)
			{
				return typeof(DateTime);
			}
			if ((object)typeName.Name == hexBinaryID)
			{
				return typeof(byte[]);
			}
			if ((object)typeName.Name == base64BinaryID)
			{
				return typeof(byte[]);
			}
			if ((object)typeName.Name == unsignedByteID)
			{
				return typeof(byte);
			}
			if ((object)typeName.Name == byteID)
			{
				return typeof(sbyte);
			}
			if ((object)typeName.Name == unsignedShortID)
			{
				return typeof(ushort);
			}
			if ((object)typeName.Name == unsignedIntID)
			{
				return typeof(uint);
			}
			if ((object)typeName.Name == unsignedLongID)
			{
				return typeof(ulong);
			}
			throw CreateUnknownTypeException(typeName);
		}
		if ((object)typeName.Namespace == schemaNs2000ID || (object)typeName.Namespace == schemaNs1999ID)
		{
			if ((object)typeName.Name == stringID || (object)typeName.Name == anyURIID || (object)typeName.Name == durationID || (object)typeName.Name == ENTITYID || (object)typeName.Name == ENTITIESID || (object)typeName.Name == gDayID || (object)typeName.Name == gMonthID || (object)typeName.Name == gMonthDayID || (object)typeName.Name == gYearID || (object)typeName.Name == gYearMonthID || (object)typeName.Name == IDID || (object)typeName.Name == IDREFID || (object)typeName.Name == IDREFSID || (object)typeName.Name == integerID || (object)typeName.Name == languageID || (object)typeName.Name == NameID || (object)typeName.Name == NCNameID || (object)typeName.Name == NMTOKENID || (object)typeName.Name == NMTOKENSID || (object)typeName.Name == negativeIntegerID || (object)typeName.Name == nonPositiveIntegerID || (object)typeName.Name == nonNegativeIntegerID || (object)typeName.Name == normalizedStringID || (object)typeName.Name == NOTATIONID || (object)typeName.Name == positiveIntegerID || (object)typeName.Name == tokenID)
			{
				return typeof(string);
			}
			if ((object)typeName.Name == intID)
			{
				return typeof(int);
			}
			if ((object)typeName.Name == booleanID)
			{
				return typeof(bool);
			}
			if ((object)typeName.Name == shortID)
			{
				return typeof(short);
			}
			if ((object)typeName.Name == longID)
			{
				return typeof(long);
			}
			if ((object)typeName.Name == floatID)
			{
				return typeof(float);
			}
			if ((object)typeName.Name == doubleID)
			{
				return typeof(double);
			}
			if ((object)typeName.Name == oldDecimalID)
			{
				return typeof(decimal);
			}
			if ((object)typeName.Name == oldTimeInstantID)
			{
				return typeof(DateTime);
			}
			if ((object)typeName.Name == qnameID)
			{
				return typeof(XmlQualifiedName);
			}
			if ((object)typeName.Name == dateID)
			{
				return typeof(DateTime);
			}
			if ((object)typeName.Name == timeID)
			{
				return typeof(DateTime);
			}
			if ((object)typeName.Name == hexBinaryID)
			{
				return typeof(byte[]);
			}
			if ((object)typeName.Name == byteID)
			{
				return typeof(sbyte);
			}
			if ((object)typeName.Name == unsignedShortID)
			{
				return typeof(ushort);
			}
			if ((object)typeName.Name == unsignedIntID)
			{
				return typeof(uint);
			}
			if ((object)typeName.Name == unsignedLongID)
			{
				return typeof(ulong);
			}
			throw CreateUnknownTypeException(typeName);
		}
		if ((object)typeName.Namespace == schemaNonXsdTypesNsID)
		{
			if ((object)typeName.Name == charID)
			{
				return typeof(char);
			}
			if ((object)typeName.Name == guidID)
			{
				return typeof(Guid);
			}
			throw CreateUnknownTypeException(typeName);
		}
		if (throwOnUnknown)
		{
			throw CreateUnknownTypeException(typeName);
		}
		return null;
	}

	private bool IsPrimitiveNamespace(string ns)
	{
		if ((object)ns != schemaNsID && (object)ns != schemaNonXsdTypesNsID && (object)ns != soapNsID && (object)ns != soap12NsID && (object)ns != schemaNs2000ID)
		{
			return (object)ns == schemaNs1999ID;
		}
		return true;
	}

	private string ReadStringValue()
	{
		if (r.IsEmptyElement)
		{
			r.Skip();
			return string.Empty;
		}
		r.ReadStartElement();
		string result = r.ReadString();
		ReadEndElement();
		return result;
	}

	private XmlQualifiedName ReadXmlQualifiedName()
	{
		bool flag = false;
		string value;
		if (r.IsEmptyElement)
		{
			value = string.Empty;
			flag = true;
		}
		else
		{
			r.ReadStartElement();
			value = r.ReadString();
		}
		XmlQualifiedName result = ToXmlQualifiedName(value);
		if (flag)
		{
			r.Skip();
			return result;
		}
		ReadEndElement();
		return result;
	}

	private byte[] ReadByteArray(bool isBase64)
	{
		ArrayList arrayList = new ArrayList();
		int num = 1024;
		int num2 = -1;
		int num3 = 0;
		int num4 = 0;
		byte[] array = new byte[num];
		arrayList.Add(array);
		while (num2 != 0)
		{
			if (num3 == array.Length)
			{
				num = Math.Min(num * 2, 65536);
				array = new byte[num];
				num3 = 0;
				arrayList.Add(array);
			}
			num2 = ((!isBase64) ? r.ReadElementContentAsBinHex(array, num3, array.Length - num3) : r.ReadElementContentAsBase64(array, num3, array.Length - num3));
			num3 += num2;
			num4 += num2;
		}
		byte[] array2 = new byte[num4];
		num3 = 0;
		foreach (byte[] item in arrayList)
		{
			num = Math.Min(item.Length, num4);
			if (num > 0)
			{
				Buffer.BlockCopy(item, 0, array2, num3, num);
				num3 += num;
				num4 -= num;
			}
		}
		arrayList.Clear();
		return array2;
	}

	protected object ReadTypedPrimitive(XmlQualifiedName type)
	{
		return ReadTypedPrimitive(type, elementCanBeType: false);
	}

	private object ReadTypedPrimitive(XmlQualifiedName type, bool elementCanBeType)
	{
		InitPrimitiveIDs();
		object obj = null;
		if (!IsPrimitiveNamespace(type.Namespace) || (object)type.Name == urTypeID)
		{
			return ReadXmlNodes(elementCanBeType);
		}
		if ((object)type.Namespace == schemaNsID || (object)type.Namespace == soapNsID || (object)type.Namespace == soap12NsID)
		{
			if ((object)type.Name == stringID || (object)type.Name == normalizedStringID)
			{
				return ReadStringValue();
			}
			if ((object)type.Name == anyURIID || (object)type.Name == durationID || (object)type.Name == ENTITYID || (object)type.Name == ENTITIESID || (object)type.Name == gDayID || (object)type.Name == gMonthID || (object)type.Name == gMonthDayID || (object)type.Name == gYearID || (object)type.Name == gYearMonthID || (object)type.Name == IDID || (object)type.Name == IDREFID || (object)type.Name == IDREFSID || (object)type.Name == integerID || (object)type.Name == languageID || (object)type.Name == NameID || (object)type.Name == NCNameID || (object)type.Name == NMTOKENID || (object)type.Name == NMTOKENSID || (object)type.Name == negativeIntegerID || (object)type.Name == nonPositiveIntegerID || (object)type.Name == nonNegativeIntegerID || (object)type.Name == NOTATIONID || (object)type.Name == positiveIntegerID || (object)type.Name == tokenID)
			{
				return CollapseWhitespace(ReadStringValue());
			}
			if ((object)type.Name == intID)
			{
				return XmlConvert.ToInt32(ReadStringValue());
			}
			if ((object)type.Name == booleanID)
			{
				return XmlConvert.ToBoolean(ReadStringValue());
			}
			if ((object)type.Name == shortID)
			{
				return XmlConvert.ToInt16(ReadStringValue());
			}
			if ((object)type.Name == longID)
			{
				return XmlConvert.ToInt64(ReadStringValue());
			}
			if ((object)type.Name == floatID)
			{
				return XmlConvert.ToSingle(ReadStringValue());
			}
			if ((object)type.Name == doubleID)
			{
				return XmlConvert.ToDouble(ReadStringValue());
			}
			if ((object)type.Name == decimalID)
			{
				return XmlConvert.ToDecimal(ReadStringValue());
			}
			if ((object)type.Name == dateTimeID)
			{
				return ToDateTime(ReadStringValue());
			}
			if ((object)type.Name == qnameID)
			{
				return ReadXmlQualifiedName();
			}
			if ((object)type.Name == dateID)
			{
				return ToDate(ReadStringValue());
			}
			if ((object)type.Name == timeID)
			{
				return ToTime(ReadStringValue());
			}
			if ((object)type.Name == unsignedByteID)
			{
				return XmlConvert.ToByte(ReadStringValue());
			}
			if ((object)type.Name == byteID)
			{
				return XmlConvert.ToSByte(ReadStringValue());
			}
			if ((object)type.Name == unsignedShortID)
			{
				return XmlConvert.ToUInt16(ReadStringValue());
			}
			if ((object)type.Name == unsignedIntID)
			{
				return XmlConvert.ToUInt32(ReadStringValue());
			}
			if ((object)type.Name == unsignedLongID)
			{
				return XmlConvert.ToUInt64(ReadStringValue());
			}
			if ((object)type.Name == hexBinaryID)
			{
				return ToByteArrayHex(isNull: false);
			}
			if ((object)type.Name == base64BinaryID)
			{
				return ToByteArrayBase64(isNull: false);
			}
			if ((object)type.Name == base64ID && ((object)type.Namespace == soapNsID || (object)type.Namespace == soap12NsID))
			{
				return ToByteArrayBase64(isNull: false);
			}
			return ReadXmlNodes(elementCanBeType);
		}
		if ((object)type.Namespace == schemaNs2000ID || (object)type.Namespace == schemaNs1999ID)
		{
			if ((object)type.Name == stringID || (object)type.Name == normalizedStringID)
			{
				return ReadStringValue();
			}
			if ((object)type.Name == anyURIID || (object)type.Name == anyURIID || (object)type.Name == durationID || (object)type.Name == ENTITYID || (object)type.Name == ENTITIESID || (object)type.Name == gDayID || (object)type.Name == gMonthID || (object)type.Name == gMonthDayID || (object)type.Name == gYearID || (object)type.Name == gYearMonthID || (object)type.Name == IDID || (object)type.Name == IDREFID || (object)type.Name == IDREFSID || (object)type.Name == integerID || (object)type.Name == languageID || (object)type.Name == NameID || (object)type.Name == NCNameID || (object)type.Name == NMTOKENID || (object)type.Name == NMTOKENSID || (object)type.Name == negativeIntegerID || (object)type.Name == nonPositiveIntegerID || (object)type.Name == nonNegativeIntegerID || (object)type.Name == NOTATIONID || (object)type.Name == positiveIntegerID || (object)type.Name == tokenID)
			{
				return CollapseWhitespace(ReadStringValue());
			}
			if ((object)type.Name == intID)
			{
				return XmlConvert.ToInt32(ReadStringValue());
			}
			if ((object)type.Name == booleanID)
			{
				return XmlConvert.ToBoolean(ReadStringValue());
			}
			if ((object)type.Name == shortID)
			{
				return XmlConvert.ToInt16(ReadStringValue());
			}
			if ((object)type.Name == longID)
			{
				return XmlConvert.ToInt64(ReadStringValue());
			}
			if ((object)type.Name == floatID)
			{
				return XmlConvert.ToSingle(ReadStringValue());
			}
			if ((object)type.Name == doubleID)
			{
				return XmlConvert.ToDouble(ReadStringValue());
			}
			if ((object)type.Name == oldDecimalID)
			{
				return XmlConvert.ToDecimal(ReadStringValue());
			}
			if ((object)type.Name == oldTimeInstantID)
			{
				return ToDateTime(ReadStringValue());
			}
			if ((object)type.Name == qnameID)
			{
				return ReadXmlQualifiedName();
			}
			if ((object)type.Name == dateID)
			{
				return ToDate(ReadStringValue());
			}
			if ((object)type.Name == timeID)
			{
				return ToTime(ReadStringValue());
			}
			if ((object)type.Name == unsignedByteID)
			{
				return XmlConvert.ToByte(ReadStringValue());
			}
			if ((object)type.Name == byteID)
			{
				return XmlConvert.ToSByte(ReadStringValue());
			}
			if ((object)type.Name == unsignedShortID)
			{
				return XmlConvert.ToUInt16(ReadStringValue());
			}
			if ((object)type.Name == unsignedIntID)
			{
				return XmlConvert.ToUInt32(ReadStringValue());
			}
			if ((object)type.Name == unsignedLongID)
			{
				return XmlConvert.ToUInt64(ReadStringValue());
			}
			return ReadXmlNodes(elementCanBeType);
		}
		if ((object)type.Namespace == schemaNonXsdTypesNsID)
		{
			if ((object)type.Name == charID)
			{
				return ToChar(ReadStringValue());
			}
			if ((object)type.Name == guidID)
			{
				return new Guid(CollapseWhitespace(ReadStringValue()));
			}
			if ((object)type.Name == timeSpanID && System.LocalAppContextSwitches.EnableTimeSpanSerialization)
			{
				return XmlConvert.ToTimeSpan(ReadStringValue());
			}
			return ReadXmlNodes(elementCanBeType);
		}
		return ReadXmlNodes(elementCanBeType);
	}

	protected object ReadTypedNull(XmlQualifiedName type)
	{
		InitPrimitiveIDs();
		object obj = null;
		if (!IsPrimitiveNamespace(type.Namespace) || (object)type.Name == urTypeID)
		{
			return null;
		}
		if ((object)type.Namespace == schemaNsID || (object)type.Namespace == soapNsID || (object)type.Namespace == soap12NsID)
		{
			if ((object)type.Name == stringID || (object)type.Name == anyURIID || (object)type.Name == durationID || (object)type.Name == ENTITYID || (object)type.Name == ENTITIESID || (object)type.Name == gDayID || (object)type.Name == gMonthID || (object)type.Name == gMonthDayID || (object)type.Name == gYearID || (object)type.Name == gYearMonthID || (object)type.Name == IDID || (object)type.Name == IDREFID || (object)type.Name == IDREFSID || (object)type.Name == integerID || (object)type.Name == languageID || (object)type.Name == NameID || (object)type.Name == NCNameID || (object)type.Name == NMTOKENID || (object)type.Name == NMTOKENSID || (object)type.Name == negativeIntegerID || (object)type.Name == nonPositiveIntegerID || (object)type.Name == nonNegativeIntegerID || (object)type.Name == normalizedStringID || (object)type.Name == NOTATIONID || (object)type.Name == positiveIntegerID || (object)type.Name == tokenID)
			{
				return null;
			}
			if ((object)type.Name == intID)
			{
				return null;
			}
			if ((object)type.Name == booleanID)
			{
				return null;
			}
			if ((object)type.Name == shortID)
			{
				return null;
			}
			if ((object)type.Name == longID)
			{
				return null;
			}
			if ((object)type.Name == floatID)
			{
				return null;
			}
			if ((object)type.Name == doubleID)
			{
				return null;
			}
			if ((object)type.Name == decimalID)
			{
				return null;
			}
			if ((object)type.Name == dateTimeID)
			{
				return null;
			}
			if ((object)type.Name == qnameID)
			{
				return null;
			}
			if ((object)type.Name == dateID)
			{
				return null;
			}
			if ((object)type.Name == timeID)
			{
				return null;
			}
			if ((object)type.Name == unsignedByteID)
			{
				return null;
			}
			if ((object)type.Name == byteID)
			{
				return null;
			}
			if ((object)type.Name == unsignedShortID)
			{
				return null;
			}
			if ((object)type.Name == unsignedIntID)
			{
				return null;
			}
			if ((object)type.Name == unsignedLongID)
			{
				return null;
			}
			if ((object)type.Name == hexBinaryID)
			{
				return null;
			}
			if ((object)type.Name == base64BinaryID)
			{
				return null;
			}
			if ((object)type.Name == base64ID && ((object)type.Namespace == soapNsID || (object)type.Namespace == soap12NsID))
			{
				return null;
			}
			return null;
		}
		if ((object)type.Namespace == schemaNonXsdTypesNsID)
		{
			if ((object)type.Name == charID)
			{
				return null;
			}
			if ((object)type.Name == guidID)
			{
				return null;
			}
			if ((object)type.Name == timeSpanID && System.LocalAppContextSwitches.EnableTimeSpanSerialization)
			{
				return null;
			}
			return null;
		}
		return null;
	}

	protected bool IsXmlnsAttribute(string name)
	{
		if (!name.StartsWith("xmlns", StringComparison.Ordinal))
		{
			return false;
		}
		if (name.Length == 5)
		{
			return true;
		}
		return name[5] == ':';
	}

	protected void ParseWsdlArrayType(XmlAttribute attr)
	{
		if ((object)attr.LocalName == wsdlArrayTypeID && (object)attr.NamespaceURI == wsdlNsID)
		{
			int num = attr.Value.LastIndexOf(':');
			if (num < 0)
			{
				attr.Value = r.LookupNamespace("") + ":" + attr.Value;
			}
			else
			{
				attr.Value = r.LookupNamespace(attr.Value.Substring(0, num)) + ":" + attr.Value.Substring(num + 1);
			}
		}
	}

	protected bool ReadNull()
	{
		if (!GetNullAttr())
		{
			return false;
		}
		if (r.IsEmptyElement)
		{
			r.Skip();
			return true;
		}
		r.ReadStartElement();
		int whileIterations = 0;
		int readerCount = ReaderCount;
		while (r.NodeType != XmlNodeType.EndElement)
		{
			UnknownNode(null);
			CheckReaderCount(ref whileIterations, ref readerCount);
		}
		ReadEndElement();
		return true;
	}

	protected bool GetNullAttr()
	{
		string attribute = r.GetAttribute(nilID, instanceNsID);
		if (attribute == null)
		{
			attribute = r.GetAttribute(nullID, instanceNsID);
		}
		if (attribute == null)
		{
			attribute = r.GetAttribute(nullID, instanceNs2000ID);
			if (attribute == null)
			{
				attribute = r.GetAttribute(nullID, instanceNs1999ID);
			}
		}
		if (attribute == null || !XmlConvert.ToBoolean(attribute))
		{
			return false;
		}
		return true;
	}

	protected string ReadNullableString()
	{
		if (ReadNull())
		{
			return null;
		}
		return r.ReadElementString();
	}

	protected XmlQualifiedName ReadNullableQualifiedName()
	{
		if (ReadNull())
		{
			return null;
		}
		return ReadElementQualifiedName();
	}

	protected XmlQualifiedName ReadElementQualifiedName()
	{
		if (r.IsEmptyElement)
		{
			XmlQualifiedName result = new XmlQualifiedName(string.Empty, r.LookupNamespace(""));
			r.Skip();
			return result;
		}
		XmlQualifiedName result2 = ToXmlQualifiedName(CollapseWhitespace(r.ReadString()));
		r.ReadEndElement();
		return result2;
	}

	protected XmlDocument ReadXmlDocument(bool wrapped)
	{
		XmlNode xmlNode = ReadXmlNode(wrapped);
		if (xmlNode == null)
		{
			return null;
		}
		XmlDocument xmlDocument = new XmlDocument();
		xmlDocument.AppendChild(xmlDocument.ImportNode(xmlNode, deep: true));
		return xmlDocument;
	}

	protected string CollapseWhitespace(string value)
	{
		return value?.Trim();
	}

	protected XmlNode ReadXmlNode(bool wrapped)
	{
		XmlNode result = null;
		if (wrapped)
		{
			if (ReadNull())
			{
				return null;
			}
			r.ReadStartElement();
			r.MoveToContent();
			if (r.NodeType != XmlNodeType.EndElement)
			{
				result = Document.ReadNode(r);
			}
			int whileIterations = 0;
			int readerCount = ReaderCount;
			while (r.NodeType != XmlNodeType.EndElement)
			{
				UnknownNode(null);
				CheckReaderCount(ref whileIterations, ref readerCount);
			}
			r.ReadEndElement();
		}
		else
		{
			result = Document.ReadNode(r);
		}
		return result;
	}

	protected static byte[] ToByteArrayBase64(string value)
	{
		return XmlCustomFormatter.ToByteArrayBase64(value);
	}

	protected byte[] ToByteArrayBase64(bool isNull)
	{
		if (isNull)
		{
			return null;
		}
		return ReadByteArray(isBase64: true);
	}

	protected static byte[] ToByteArrayHex(string value)
	{
		return XmlCustomFormatter.ToByteArrayHex(value);
	}

	protected byte[] ToByteArrayHex(bool isNull)
	{
		if (isNull)
		{
			return null;
		}
		return ReadByteArray(isBase64: false);
	}

	protected int GetArrayLength(string name, string ns)
	{
		if (GetNullAttr())
		{
			return 0;
		}
		string attribute = r.GetAttribute(arrayTypeID, soapNsID);
		SoapArrayInfo soapArrayInfo = ParseArrayType(attribute);
		if (soapArrayInfo.dimensions != 1)
		{
			throw new InvalidOperationException(Res.GetString("SOAP-ENC:arrayType with multidimensional array found at {0}. Only single-dimensional arrays are supported. Consider using an array of arrays instead.", CurrentTag()));
		}
		XmlQualifiedName xmlQualifiedName = ToXmlQualifiedName(soapArrayInfo.qname, decodeName: false);
		if (xmlQualifiedName.Name != name)
		{
			throw new InvalidOperationException(Res.GetString("The SOAP-ENC:arrayType references type is named '{0}'; a type named '{1}' was expected at {2}.", xmlQualifiedName.Name, name, CurrentTag()));
		}
		if (xmlQualifiedName.Namespace != ns)
		{
			throw new InvalidOperationException(Res.GetString("The SOAP-ENC:arrayType references type is from namespace '{0}'; the namespace '{1}' was expected at {2}.", xmlQualifiedName.Namespace, ns, CurrentTag()));
		}
		return soapArrayInfo.length;
	}

	private SoapArrayInfo ParseArrayType(string value)
	{
		if (value == null)
		{
			throw new ArgumentNullException(Res.GetString("SOAP-ENC:arrayType was missing at {0}.", CurrentTag()));
		}
		if (value.Length == 0)
		{
			throw new ArgumentException(Res.GetString("SOAP-ENC:arrayType was empty at {0}.", CurrentTag()), "value");
		}
		char[] array = value.ToCharArray();
		int num = array.Length;
		SoapArrayInfo result = default(SoapArrayInfo);
		int num2 = num - 1;
		if (array[num2] != ']')
		{
			throw new ArgumentException(Res.GetString("SOAP-ENC:arrayType must end with a ']' character."), "value");
		}
		num2--;
		while (num2 != -1 && array[num2] != '[')
		{
			if (array[num2] == ',')
			{
				throw new ArgumentException(Res.GetString("SOAP-ENC:arrayType with multidimensional array found at {0}. Only single-dimensional arrays are supported. Consider using an array of arrays instead.", CurrentTag()), "value");
			}
			num2--;
		}
		if (num2 == -1)
		{
			throw new ArgumentException(Res.GetString("SOAP-ENC:arrayType has mismatched brackets."), "value");
		}
		int num3 = num - num2 - 2;
		if (num3 > 0)
		{
			string text = new string(array, num2 + 1, num3);
			try
			{
				result.length = int.Parse(text, CultureInfo.InvariantCulture);
			}
			catch (Exception ex)
			{
				if (ex is ThreadAbortException || ex is StackOverflowException || ex is OutOfMemoryException)
				{
					throw;
				}
				throw new ArgumentException(Res.GetString("SOAP-ENC:arrayType could not handle '{1}' as the length of the array.", text), "value");
			}
		}
		else
		{
			result.length = -1;
		}
		num2--;
		result.jaggedDimensions = 0;
		while (num2 != -1 && array[num2] == ']')
		{
			num2--;
			if (num2 < 0)
			{
				throw new ArgumentException(Res.GetString("SOAP-ENC:arrayType has mismatched brackets."), "value");
			}
			if (array[num2] == ',')
			{
				throw new ArgumentException(Res.GetString("SOAP-ENC:arrayType with multidimensional array found at {0}. Only single-dimensional arrays are supported. Consider using an array of arrays instead.", CurrentTag()), "value");
			}
			if (array[num2] != '[')
			{
				throw new ArgumentException(Res.GetString("SOAP-ENC:arrayType must end with a ']' character."), "value");
			}
			num2--;
			result.jaggedDimensions++;
		}
		result.dimensions = 1;
		result.qname = new string(array, 0, num2 + 1);
		return result;
	}

	private SoapArrayInfo ParseSoap12ArrayType(string itemType, string arraySize)
	{
		SoapArrayInfo result = default(SoapArrayInfo);
		if (itemType != null && itemType.Length > 0)
		{
			result.qname = itemType;
		}
		else
		{
			result.qname = "";
		}
		string[] array = ((arraySize == null || arraySize.Length <= 0) ? new string[0] : arraySize.Split(null));
		result.dimensions = 0;
		result.length = -1;
		for (int i = 0; i < array.Length; i++)
		{
			if (array[i].Length <= 0)
			{
				continue;
			}
			if (array[i] == "*")
			{
				result.dimensions++;
				continue;
			}
			try
			{
				result.length = int.Parse(array[i], CultureInfo.InvariantCulture);
				result.dimensions++;
			}
			catch (Exception ex)
			{
				if (ex is ThreadAbortException || ex is StackOverflowException || ex is OutOfMemoryException)
				{
					throw;
				}
				throw new ArgumentException(Res.GetString("SOAP-ENC:arrayType could not handle '{1}' as the length of the array.", array[i]), "value");
			}
		}
		if (result.dimensions == 0)
		{
			result.dimensions = 1;
		}
		return result;
	}

	protected static DateTime ToDateTime(string value)
	{
		return XmlCustomFormatter.ToDateTime(value);
	}

	protected static DateTime ToDate(string value)
	{
		return XmlCustomFormatter.ToDate(value);
	}

	protected static DateTime ToTime(string value)
	{
		return XmlCustomFormatter.ToTime(value);
	}

	protected static char ToChar(string value)
	{
		return XmlCustomFormatter.ToChar(value);
	}

	protected static long ToEnum(string value, Hashtable h, string typeName)
	{
		return XmlCustomFormatter.ToEnum(value, h, typeName, validate: true);
	}

	protected static string ToXmlName(string value)
	{
		return XmlCustomFormatter.ToXmlName(value);
	}

	protected static string ToXmlNCName(string value)
	{
		return XmlCustomFormatter.ToXmlNCName(value);
	}

	protected static string ToXmlNmToken(string value)
	{
		return XmlCustomFormatter.ToXmlNmToken(value);
	}

	protected static string ToXmlNmTokens(string value)
	{
		return XmlCustomFormatter.ToXmlNmTokens(value);
	}

	protected XmlQualifiedName ToXmlQualifiedName(string value)
	{
		return ToXmlQualifiedName(value, DecodeName);
	}

	internal XmlQualifiedName ToXmlQualifiedName(string value, bool decodeName)
	{
		int num = value?.LastIndexOf(':') ?? (-1);
		string text = ((num < 0) ? null : value.Substring(0, num));
		string text2 = value.Substring(num + 1);
		if (decodeName)
		{
			text = XmlConvert.DecodeName(text);
			text2 = XmlConvert.DecodeName(text2);
		}
		if (text == null || text.Length == 0)
		{
			return new XmlQualifiedName(r.NameTable.Add(value), r.LookupNamespace(string.Empty));
		}
		string text3 = r.LookupNamespace(text);
		if (text3 == null)
		{
			throw new InvalidOperationException(Res.GetString("Namespace prefix '{0}' is not defined.", text));
		}
		return new XmlQualifiedName(r.NameTable.Add(text2), text3);
	}

	protected void UnknownAttribute(object o, XmlAttribute attr)
	{
		UnknownAttribute(o, attr, null);
	}

	protected void UnknownAttribute(object o, XmlAttribute attr, string qnames)
	{
		if (events.OnUnknownAttribute != null)
		{
			GetCurrentPosition(out var lineNumber, out var linePosition);
			XmlAttributeEventArgs e = new XmlAttributeEventArgs(attr, lineNumber, linePosition, o, qnames);
			events.OnUnknownAttribute(events.sender, e);
		}
	}

	protected void UnknownElement(object o, XmlElement elem)
	{
		UnknownElement(o, elem, null);
	}

	protected void UnknownElement(object o, XmlElement elem, string qnames)
	{
		if (events.OnUnknownElement != null)
		{
			GetCurrentPosition(out var lineNumber, out var linePosition);
			XmlElementEventArgs e = new XmlElementEventArgs(elem, lineNumber, linePosition, o, qnames);
			events.OnUnknownElement(events.sender, e);
		}
	}

	protected void UnknownNode(object o)
	{
		UnknownNode(o, null);
	}

	protected void UnknownNode(object o, string qnames)
	{
		if (r.NodeType == XmlNodeType.None || r.NodeType == XmlNodeType.Whitespace)
		{
			r.Read();
		}
		else
		{
			if (r.NodeType == XmlNodeType.EndElement)
			{
				return;
			}
			if (events.OnUnknownNode != null)
			{
				UnknownNode(Document.ReadNode(r), o, qnames);
			}
			else if (r.NodeType != XmlNodeType.Attribute || events.OnUnknownAttribute != null)
			{
				if (r.NodeType == XmlNodeType.Element && events.OnUnknownElement == null)
				{
					r.Skip();
				}
				else
				{
					UnknownNode(Document.ReadNode(r), o, qnames);
				}
			}
		}
	}

	private void UnknownNode(XmlNode unknownNode, object o, string qnames)
	{
		if (unknownNode != null)
		{
			if (unknownNode.NodeType != 0 && unknownNode.NodeType != XmlNodeType.Whitespace && events.OnUnknownNode != null)
			{
				GetCurrentPosition(out var lineNumber, out var linePosition);
				XmlNodeEventArgs e = new XmlNodeEventArgs(unknownNode, lineNumber, linePosition, o);
				events.OnUnknownNode(events.sender, e);
			}
			if (unknownNode.NodeType == XmlNodeType.Attribute)
			{
				UnknownAttribute(o, (XmlAttribute)unknownNode, qnames);
			}
			else if (unknownNode.NodeType == XmlNodeType.Element)
			{
				UnknownElement(o, (XmlElement)unknownNode, qnames);
			}
		}
	}

	private void GetCurrentPosition(out int lineNumber, out int linePosition)
	{
		if (Reader is IXmlLineInfo)
		{
			IXmlLineInfo xmlLineInfo = (IXmlLineInfo)Reader;
			lineNumber = xmlLineInfo.LineNumber;
			linePosition = xmlLineInfo.LinePosition;
		}
		else
		{
			lineNumber = (linePosition = -1);
		}
	}

	protected void UnreferencedObject(string id, object o)
	{
		if (events.OnUnreferencedObject != null)
		{
			UnreferencedObjectEventArgs e = new UnreferencedObjectEventArgs(o, id);
			events.OnUnreferencedObject(events.sender, e);
		}
	}

	private string CurrentTag()
	{
		return r.NodeType switch
		{
			XmlNodeType.Element => "<" + r.LocalName + " xmlns='" + r.NamespaceURI + "'>", 
			XmlNodeType.EndElement => ">", 
			XmlNodeType.Text => r.Value, 
			XmlNodeType.CDATA => "CDATA", 
			XmlNodeType.Comment => "<--", 
			XmlNodeType.ProcessingInstruction => "<?", 
			_ => "(unknown)", 
		};
	}

	protected Exception CreateUnknownTypeException(XmlQualifiedName type)
	{
		return new InvalidOperationException(Res.GetString("The specified type was not recognized: name='{0}', namespace='{1}', at {2}.", type.Name, type.Namespace, CurrentTag()));
	}

	protected Exception CreateReadOnlyCollectionException(string name)
	{
		return new InvalidOperationException(Res.GetString("Could not deserialize {0}. Parameterless constructor is required for collections and enumerators.", name));
	}

	protected Exception CreateAbstractTypeException(string name, string ns)
	{
		return new InvalidOperationException(Res.GetString("The specified type is abstract: name='{0}', namespace='{1}', at {2}.", name, ns, CurrentTag()));
	}

	protected Exception CreateInaccessibleConstructorException(string typeName)
	{
		return new InvalidOperationException(Res.GetString("{0} cannot be serialized because it does not have a parameterless constructor.", typeName));
	}

	protected Exception CreateCtorHasSecurityException(string typeName)
	{
		return new InvalidOperationException(Res.GetString("The type '{0}' cannot be serialized because its parameterless constructor is decorated with declarative security permission attributes. Consider using imperative asserts or demands in the constructor.", typeName));
	}

	protected Exception CreateUnknownNodeException()
	{
		return new InvalidOperationException(Res.GetString("{0} was not expected.", CurrentTag()));
	}

	protected Exception CreateUnknownConstantException(string value, Type enumType)
	{
		return new InvalidOperationException(Res.GetString("Instance validation error: '{0}' is not a valid value for {1}.", value, enumType.Name));
	}

	protected Exception CreateInvalidCastException(Type type, object value)
	{
		return CreateInvalidCastException(type, value, null);
	}

	protected Exception CreateInvalidCastException(Type type, object value, string id)
	{
		if (value == null)
		{
			return new InvalidCastException(Res.GetString("Cannot assign null value to an object of type {1}.", type.FullName));
		}
		if (id == null)
		{
			return new InvalidCastException(Res.GetString("Cannot assign object of type {0} to an object of type {1}.", value.GetType().FullName, type.FullName));
		}
		return new InvalidCastException(Res.GetString("Cannot assign object of type {0} to an object of type {1}. The error occurred while reading node with id='{2}'.", value.GetType().FullName, type.FullName, id));
	}

	protected Exception CreateBadDerivationException(string xsdDerived, string nsDerived, string xsdBase, string nsBase, string clrDerived, string clrBase)
	{
		return new InvalidOperationException(Res.GetString("Type '{0}' from namespace '{1}' declared as derivation of type '{2}' from namespace '{3}, but corresponding CLR types are not compatible.  Cannot convert type '{4}' to '{5}'.", xsdDerived, nsDerived, xsdBase, nsBase, clrDerived, clrBase));
	}

	protected Exception CreateMissingIXmlSerializableType(string name, string ns, string clrType)
	{
		return new InvalidOperationException(Res.GetString("Type '{0}' from namespace '{1}' does not have corresponding IXmlSerializable type. Please consider adding {2} to '{3}'.", name, ns, typeof(XmlIncludeAttribute).Name, clrType));
	}

	protected Array EnsureArrayIndex(Array a, int index, Type elementType)
	{
		if (a == null)
		{
			return Array.CreateInstance(elementType, 32);
		}
		if (index < a.Length)
		{
			return a;
		}
		Array array = Array.CreateInstance(elementType, a.Length * 2);
		Array.Copy(a, array, index);
		return array;
	}

	protected Array ShrinkArray(Array a, int length, Type elementType, bool isNullable)
	{
		if (a == null)
		{
			if (isNullable)
			{
				return null;
			}
			return Array.CreateInstance(elementType, 0);
		}
		if (a.Length == length)
		{
			return a;
		}
		Array array = Array.CreateInstance(elementType, length);
		Array.Copy(a, array, length);
		return array;
	}

	protected string ReadString(string value)
	{
		return ReadString(value, trim: false);
	}

	protected string ReadString(string value, bool trim)
	{
		string text = r.ReadString();
		if (text != null && trim)
		{
			text = text.Trim();
		}
		if (value == null || value.Length == 0)
		{
			return text;
		}
		return value + text;
	}

	protected IXmlSerializable ReadSerializable(IXmlSerializable serializable)
	{
		return ReadSerializable(serializable, wrappedAny: false);
	}

	protected IXmlSerializable ReadSerializable(IXmlSerializable serializable, bool wrappedAny)
	{
		string text = null;
		string text2 = null;
		if (wrappedAny)
		{
			text = r.LocalName;
			text2 = r.NamespaceURI;
			r.Read();
			r.MoveToContent();
		}
		serializable.ReadXml(r);
		if (wrappedAny)
		{
			while (r.NodeType == XmlNodeType.Whitespace)
			{
				r.Skip();
			}
			if (r.NodeType == XmlNodeType.None)
			{
				r.Skip();
			}
			if (r.NodeType == XmlNodeType.EndElement && r.LocalName == text && r.NamespaceURI == text2)
			{
				Reader.Read();
			}
		}
		return serializable;
	}

	protected bool ReadReference(out string fixupReference)
	{
		string text = (soap12 ? r.GetAttribute("ref", "http://www.w3.org/2003/05/soap-encoding") : r.GetAttribute("href"));
		if (text == null)
		{
			fixupReference = null;
			return false;
		}
		if (!soap12)
		{
			if (!text.StartsWith("#", StringComparison.Ordinal))
			{
				throw new InvalidOperationException(Res.GetString("The referenced element with ID '{0}' is located outside the current document and cannot be retrieved.", text));
			}
			fixupReference = text.Substring(1);
		}
		else
		{
			fixupReference = text;
		}
		if (r.IsEmptyElement)
		{
			r.Skip();
		}
		else
		{
			r.ReadStartElement();
			ReadEndElement();
		}
		return true;
	}

	protected void AddTarget(string id, object o)
	{
		if (id == null)
		{
			if (targetsWithoutIds == null)
			{
				targetsWithoutIds = new ArrayList();
			}
			if (o != null)
			{
				targetsWithoutIds.Add(o);
			}
		}
		else
		{
			if (targets == null)
			{
				targets = new Hashtable();
			}
			if (!targets.Contains(id))
			{
				targets.Add(id, o);
			}
		}
	}

	protected void AddFixup(Fixup fixup)
	{
		if (fixups == null)
		{
			fixups = new ArrayList();
		}
		fixups.Add(fixup);
	}

	protected void AddFixup(CollectionFixup fixup)
	{
		if (collectionFixups == null)
		{
			collectionFixups = new ArrayList();
		}
		collectionFixups.Add(fixup);
	}

	protected object GetTarget(string id)
	{
		object obj = ((targets != null) ? targets[id] : null);
		if (obj == null)
		{
			throw new InvalidOperationException(Res.GetString("The referenced element with ID '{0}' was not found in the document.", id));
		}
		Referenced(obj);
		return obj;
	}

	protected void Referenced(object o)
	{
		if (o != null)
		{
			if (referencedTargets == null)
			{
				referencedTargets = new Hashtable();
			}
			referencedTargets[o] = o;
		}
	}

	private void HandleUnreferencedObjects()
	{
		if (targets != null)
		{
			foreach (DictionaryEntry target in targets)
			{
				if (referencedTargets == null || !referencedTargets.Contains(target.Value))
				{
					UnreferencedObject((string)target.Key, target.Value);
				}
			}
		}
		if (targetsWithoutIds == null)
		{
			return;
		}
		foreach (object targetsWithoutId in targetsWithoutIds)
		{
			if (referencedTargets == null || !referencedTargets.Contains(targetsWithoutId))
			{
				UnreferencedObject(null, targetsWithoutId);
			}
		}
	}

	private void DoFixups()
	{
		if (fixups == null)
		{
			return;
		}
		for (int i = 0; i < fixups.Count; i++)
		{
			Fixup fixup = (Fixup)fixups[i];
			fixup.Callback(fixup);
		}
		if (collectionFixups != null)
		{
			for (int j = 0; j < collectionFixups.Count; j++)
			{
				CollectionFixup collectionFixup = (CollectionFixup)collectionFixups[j];
				collectionFixup.Callback(collectionFixup.Collection, collectionFixup.CollectionItems);
			}
		}
	}

	protected void FixupArrayRefs(object fixup)
	{
		Fixup fixup2 = (Fixup)fixup;
		Array array = (Array)fixup2.Source;
		for (int i = 0; i < array.Length; i++)
		{
			string text = fixup2.Ids[i];
			if (text != null)
			{
				object target = GetTarget(text);
				try
				{
					array.SetValue(target, i);
				}
				catch (InvalidCastException)
				{
					throw new InvalidOperationException(Res.GetString("Invalid reference id='{0}'. Object of type {1} cannot be stored in an array of this type. Details: array index={2}.", text, target.GetType().FullName, i.ToString(CultureInfo.InvariantCulture)));
				}
			}
		}
	}

	private object ReadArray(string typeName, string typeNs)
	{
		Type type = null;
		SoapArrayInfo soapArrayInfo;
		if (soap12)
		{
			string attribute = r.GetAttribute(itemTypeID, soap12NsID);
			string attribute2 = r.GetAttribute(arraySizeID, soap12NsID);
			Type type2 = (Type)types[new XmlQualifiedName(typeName, typeNs)];
			if (attribute == null && attribute2 == null && (type2 == null || !type2.IsArray))
			{
				return null;
			}
			soapArrayInfo = ParseSoap12ArrayType(attribute, attribute2);
			if (type2 != null)
			{
				type = TypeScope.GetArrayElementType(type2, null);
			}
		}
		else
		{
			string attribute3 = r.GetAttribute(arrayTypeID, soapNsID);
			if (attribute3 == null)
			{
				return null;
			}
			soapArrayInfo = ParseArrayType(attribute3);
		}
		if (soapArrayInfo.dimensions != 1)
		{
			throw new InvalidOperationException(Res.GetString("SOAP-ENC:arrayType with multidimensional array found at {0}. Only single-dimensional arrays are supported. Consider using an array of arrays instead.", CurrentTag()));
		}
		Type type3 = null;
		XmlQualifiedName xmlQualifiedName = new XmlQualifiedName(urTypeID, schemaNsID);
		XmlQualifiedName xmlQualifiedName2;
		if (soapArrayInfo.qname.Length > 0)
		{
			xmlQualifiedName2 = ToXmlQualifiedName(soapArrayInfo.qname, decodeName: false);
			type3 = (Type)types[xmlQualifiedName2];
		}
		else
		{
			xmlQualifiedName2 = xmlQualifiedName;
		}
		if (soap12 && type3 == typeof(object))
		{
			type3 = null;
		}
		bool flag;
		if (type3 == null)
		{
			if (!soap12)
			{
				type3 = GetPrimitiveType(xmlQualifiedName2, throwOnUnknown: true);
				flag = true;
			}
			else
			{
				if (xmlQualifiedName2 != xmlQualifiedName)
				{
					type3 = GetPrimitiveType(xmlQualifiedName2, throwOnUnknown: false);
				}
				if (type3 != null)
				{
					flag = true;
				}
				else if (type == null)
				{
					type3 = typeof(object);
					flag = false;
				}
				else
				{
					type3 = type;
					XmlQualifiedName xmlQualifiedName3 = (XmlQualifiedName)typesReverse[type3];
					if (xmlQualifiedName3 == null)
					{
						xmlQualifiedName3 = XmlSerializationWriter.GetPrimitiveTypeNameInternal(type3);
						flag = true;
					}
					else
					{
						flag = type3.IsPrimitive;
					}
					if (xmlQualifiedName3 != null)
					{
						xmlQualifiedName2 = xmlQualifiedName3;
					}
				}
			}
		}
		else
		{
			flag = type3.IsPrimitive;
		}
		if (!soap12 && soapArrayInfo.jaggedDimensions > 0)
		{
			for (int i = 0; i < soapArrayInfo.jaggedDimensions; i++)
			{
				type3 = type3.MakeArrayType();
			}
		}
		if (r.IsEmptyElement)
		{
			r.Skip();
			return Array.CreateInstance(type3, 0);
		}
		r.ReadStartElement();
		r.MoveToContent();
		int num = 0;
		Array array = null;
		if (type3.IsValueType)
		{
			if (!flag && !type3.IsEnum)
			{
				throw new NotSupportedException(Res.GetString("Cannot serialize {0}. Arrays of structs are not supported with encoded SOAP.", type3.FullName));
			}
			int whileIterations = 0;
			int readerCount = ReaderCount;
			while (r.NodeType != XmlNodeType.EndElement)
			{
				array = EnsureArrayIndex(array, num, type3);
				array.SetValue(ReadReferencedElement(xmlQualifiedName2.Name, xmlQualifiedName2.Namespace), num);
				num++;
				r.MoveToContent();
				CheckReaderCount(ref whileIterations, ref readerCount);
			}
			array = ShrinkArray(array, num, type3, isNullable: false);
		}
		else
		{
			string[] array2 = null;
			int num2 = 0;
			int whileIterations2 = 0;
			int readerCount2 = ReaderCount;
			while (r.NodeType != XmlNodeType.EndElement)
			{
				array = EnsureArrayIndex(array, num, type3);
				array2 = (string[])EnsureArrayIndex(array2, num2, typeof(string));
				string name;
				string ns;
				if (r.NamespaceURI.Length != 0)
				{
					name = r.LocalName;
					ns = (((object)r.NamespaceURI != soapNsID) ? r.NamespaceURI : "http://www.w3.org/2001/XMLSchema");
				}
				else
				{
					name = xmlQualifiedName2.Name;
					ns = xmlQualifiedName2.Namespace;
				}
				array.SetValue(ReadReferencingElement(name, ns, out array2[num2]), num);
				num++;
				num2++;
				r.MoveToContent();
				CheckReaderCount(ref whileIterations2, ref readerCount2);
			}
			if (soap12 && type3 == typeof(object))
			{
				Type type4 = null;
				for (int j = 0; j < num; j++)
				{
					object value = array.GetValue(j);
					if (value != null)
					{
						Type type5 = value.GetType();
						if (type5.IsValueType)
						{
							type4 = null;
							break;
						}
						if (type4 == null || type5.IsAssignableFrom(type4))
						{
							type4 = type5;
						}
						else if (!type4.IsAssignableFrom(type5))
						{
							type4 = null;
							break;
						}
					}
				}
				if (type4 != null)
				{
					type3 = type4;
				}
			}
			array2 = (string[])ShrinkArray(array2, num2, typeof(string), isNullable: false);
			array = ShrinkArray(array, num, type3, isNullable: false);
			Fixup fixup = new Fixup(array, FixupArrayRefs, array2);
			AddFixup(fixup);
		}
		ReadEndElement();
		return array;
	}

	protected abstract void InitCallbacks();

	protected void ReadReferencedElements()
	{
		r.MoveToContent();
		int whileIterations = 0;
		int readerCount = ReaderCount;
		while (r.NodeType != XmlNodeType.EndElement && r.NodeType != 0)
		{
			ReadReferencingElement(null, null, elementCanBeType: true, out var _);
			r.MoveToContent();
			CheckReaderCount(ref whileIterations, ref readerCount);
		}
		DoFixups();
		HandleUnreferencedObjects();
	}

	protected object ReadReferencedElement()
	{
		return ReadReferencedElement(null, null);
	}

	protected object ReadReferencedElement(string name, string ns)
	{
		string fixupReference;
		return ReadReferencingElement(name, ns, out fixupReference);
	}

	protected object ReadReferencingElement(out string fixupReference)
	{
		return ReadReferencingElement(null, null, out fixupReference);
	}

	protected object ReadReferencingElement(string name, string ns, out string fixupReference)
	{
		return ReadReferencingElement(name, ns, elementCanBeType: false, out fixupReference);
	}

	protected object ReadReferencingElement(string name, string ns, bool elementCanBeType, out string fixupReference)
	{
		object obj = null;
		if (callbacks == null)
		{
			callbacks = new Hashtable();
			types = new Hashtable();
			XmlQualifiedName xmlQualifiedName = new XmlQualifiedName(urTypeID, r.NameTable.Add("http://www.w3.org/2001/XMLSchema"));
			types.Add(xmlQualifiedName, typeof(object));
			typesReverse = new Hashtable();
			typesReverse.Add(typeof(object), xmlQualifiedName);
			InitCallbacks();
		}
		r.MoveToContent();
		if (ReadReference(out fixupReference))
		{
			return null;
		}
		if (ReadNull())
		{
			return null;
		}
		string id = (soap12 ? r.GetAttribute("id", "http://www.w3.org/2003/05/soap-encoding") : r.GetAttribute("id", null));
		if ((obj = ReadArray(name, ns)) == null)
		{
			XmlQualifiedName xmlQualifiedName2 = GetXsiType();
			if (xmlQualifiedName2 == null)
			{
				xmlQualifiedName2 = ((name != null) ? new XmlQualifiedName(r.NameTable.Add(name), r.NameTable.Add(ns)) : new XmlQualifiedName(r.NameTable.Add(r.LocalName), r.NameTable.Add(r.NamespaceURI)));
			}
			XmlSerializationReadCallback xmlSerializationReadCallback = (XmlSerializationReadCallback)callbacks[xmlQualifiedName2];
			obj = ((xmlSerializationReadCallback == null) ? ReadTypedPrimitive(xmlQualifiedName2, elementCanBeType) : xmlSerializationReadCallback());
		}
		AddTarget(id, obj);
		return obj;
	}

	protected void AddReadCallback(string name, string ns, Type type, XmlSerializationReadCallback read)
	{
		XmlQualifiedName xmlQualifiedName = new XmlQualifiedName(r.NameTable.Add(name), r.NameTable.Add(ns));
		callbacks[xmlQualifiedName] = read;
		types[xmlQualifiedName] = type;
		typesReverse[type] = xmlQualifiedName;
	}

	protected void ReadEndElement()
	{
		while (r.NodeType == XmlNodeType.Whitespace)
		{
			r.Skip();
		}
		if (r.NodeType == XmlNodeType.None)
		{
			r.Skip();
		}
		else
		{
			r.ReadEndElement();
		}
	}

	private object ReadXmlNodes(bool elementCanBeType)
	{
		ArrayList arrayList = new ArrayList();
		string localName = Reader.LocalName;
		string namespaceURI = Reader.NamespaceURI;
		string name = Reader.Name;
		string text = null;
		string text2 = null;
		int num = 0;
		int lineNumber = -1;
		int linePosition = -1;
		XmlNode xmlNode = null;
		if (Reader.NodeType == XmlNodeType.Attribute)
		{
			XmlAttribute xmlAttribute = Document.CreateAttribute(name, namespaceURI);
			xmlAttribute.Value = Reader.Value;
			xmlNode = xmlAttribute;
		}
		else
		{
			xmlNode = Document.CreateElement(name, namespaceURI);
		}
		GetCurrentPosition(out lineNumber, out linePosition);
		XmlElement xmlElement = xmlNode as XmlElement;
		while (Reader.MoveToNextAttribute())
		{
			if (IsXmlnsAttribute(Reader.Name) || (Reader.Name == "id" && (!soap12 || Reader.NamespaceURI == "http://www.w3.org/2003/05/soap-encoding")))
			{
				num++;
			}
			if ((object)Reader.LocalName == typeID && ((object)Reader.NamespaceURI == instanceNsID || (object)Reader.NamespaceURI == instanceNs2000ID || (object)Reader.NamespaceURI == instanceNs1999ID))
			{
				string value = Reader.Value;
				int num2 = value.LastIndexOf(':');
				text = ((num2 >= 0) ? value.Substring(num2 + 1) : value);
				text2 = Reader.LookupNamespace((num2 >= 0) ? value.Substring(0, num2) : "");
			}
			XmlAttribute xmlAttribute2 = (XmlAttribute)Document.ReadNode(r);
			arrayList.Add(xmlAttribute2);
			xmlElement?.SetAttributeNode(xmlAttribute2);
		}
		if (elementCanBeType && text == null)
		{
			text = localName;
			text2 = namespaceURI;
			XmlAttribute xmlAttribute3 = Document.CreateAttribute(typeID, instanceNsID);
			xmlAttribute3.Value = name;
			arrayList.Add(xmlAttribute3);
		}
		if (text == "anyType" && ((object)text2 == schemaNsID || (object)text2 == schemaNs1999ID || (object)text2 == schemaNs2000ID))
		{
			num++;
		}
		Reader.MoveToElement();
		if (Reader.IsEmptyElement)
		{
			Reader.Skip();
		}
		else
		{
			Reader.ReadStartElement();
			Reader.MoveToContent();
			int whileIterations = 0;
			int readerCount = ReaderCount;
			while (Reader.NodeType != XmlNodeType.EndElement)
			{
				XmlNode xmlNode2 = Document.ReadNode(r);
				arrayList.Add(xmlNode2);
				xmlElement?.AppendChild(xmlNode2);
				Reader.MoveToContent();
				CheckReaderCount(ref whileIterations, ref readerCount);
			}
			ReadEndElement();
		}
		if (arrayList.Count <= num)
		{
			return new object();
		}
		XmlNode[] result = (XmlNode[])arrayList.ToArray(typeof(XmlNode));
		UnknownNode(xmlNode, null, null);
		return result;
	}

	protected void CheckReaderCount(ref int whileIterations, ref int readerCount)
	{
		if (!checkDeserializeAdvances)
		{
			return;
		}
		whileIterations++;
		if ((whileIterations & 0x80) == 128)
		{
			if (readerCount == ReaderCount)
			{
				throw new InvalidOperationException(Res.GetString("Internal error: deserialization failed to advance over underlying stream."));
			}
			readerCount = ReaderCount;
		}
	}
}
