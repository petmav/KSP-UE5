using System.Reflection;
using System.Xml;
using System.Xml.Schema;
using System.Xml.Serialization;

namespace System.Runtime.Serialization;

internal class SchemaExporter
{
	private static XmlQualifiedName actualTypeAnnotationName;

	internal static XmlQualifiedName ActualTypeAnnotationName
	{
		get
		{
			if (actualTypeAnnotationName == null)
			{
				actualTypeAnnotationName = new XmlQualifiedName("ActualType", "http://schemas.microsoft.com/2003/10/Serialization/");
			}
			return actualTypeAnnotationName;
		}
	}

	internal static void GetXmlTypeInfo(Type type, out XmlQualifiedName stableName, out XmlSchemaType xsdType, out bool hasRoot)
	{
		if (!IsSpecialXmlType(type, out stableName, out xsdType, out hasRoot))
		{
			XmlSchemaSet xmlSchemaSet = new XmlSchemaSet();
			xmlSchemaSet.XmlResolver = null;
			InvokeSchemaProviderMethod(type, xmlSchemaSet, out stableName, out xsdType, out hasRoot);
			if (stableName.Name == null || stableName.Name.Length == 0)
			{
				throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new InvalidDataContractException(SR.GetString("XML data contract Name for type '{0}' cannot be set to null or empty string.", DataContract.GetClrTypeFullName(type))));
			}
		}
	}

	internal static bool IsSpecialXmlType(Type type, out XmlQualifiedName typeName, out XmlSchemaType xsdType, out bool hasRoot)
	{
		xsdType = null;
		hasRoot = true;
		if (type == Globals.TypeOfXmlElement || type == Globals.TypeOfXmlNodeArray)
		{
			string text = null;
			if (type == Globals.TypeOfXmlElement)
			{
				xsdType = CreateAnyElementType();
				text = "XmlElement";
				hasRoot = false;
			}
			else
			{
				xsdType = CreateAnyType();
				text = "ArrayOfXmlNode";
				hasRoot = true;
			}
			typeName = new XmlQualifiedName(text, DataContract.GetDefaultStableNamespace(type));
			return true;
		}
		typeName = null;
		return false;
	}

	internal static void AddDefaultXmlType(XmlSchemaSet schemas, string localName, string ns)
	{
		throw new NotImplementedException();
	}

	private static bool InvokeSchemaProviderMethod(Type clrType, XmlSchemaSet schemas, out XmlQualifiedName stableName, out XmlSchemaType xsdType, out bool hasRoot)
	{
		xsdType = null;
		hasRoot = true;
		object[] customAttributes = clrType.GetCustomAttributes(Globals.TypeOfXmlSchemaProviderAttribute, inherit: false);
		if (customAttributes == null || customAttributes.Length == 0)
		{
			stableName = DataContract.GetDefaultStableName(clrType);
			return false;
		}
		XmlSchemaProviderAttribute xmlSchemaProviderAttribute = (XmlSchemaProviderAttribute)customAttributes[0];
		if (xmlSchemaProviderAttribute.IsAny)
		{
			xsdType = CreateAnyElementType();
			hasRoot = false;
		}
		string methodName = xmlSchemaProviderAttribute.MethodName;
		if (methodName == null || methodName.Length == 0)
		{
			if (!xmlSchemaProviderAttribute.IsAny)
			{
				throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new InvalidDataContractException(SR.GetString("Type '{0}' cannot have MethodName on XmlSchemaProviderAttribute attribute set to null or empty string.", DataContract.GetClrTypeFullName(clrType))));
			}
			stableName = DataContract.GetDefaultStableName(clrType);
		}
		else
		{
			MethodInfo method = clrType.GetMethod(methodName, BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic, null, new Type[1] { typeof(XmlSchemaSet) }, null);
			if (method == null)
			{
				throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new InvalidDataContractException(SR.GetString("Type '{0}' does not have a static method '{1}' that takes a parameter of type 'System.Xml.Schema.XmlSchemaSet' as specified by the XmlSchemaProviderAttribute attribute.", DataContract.GetClrTypeFullName(clrType), methodName)));
			}
			if (!Globals.TypeOfXmlQualifiedName.IsAssignableFrom(method.ReturnType) && !Globals.TypeOfXmlSchemaType.IsAssignableFrom(method.ReturnType))
			{
				throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new InvalidDataContractException(SR.GetString("Method '{0}.{1}()' returns '{2}'. The return type must be compatible with '{3}'.", DataContract.GetClrTypeFullName(clrType), methodName, DataContract.GetClrTypeFullName(method.ReturnType), DataContract.GetClrTypeFullName(Globals.TypeOfXmlQualifiedName), typeof(XmlSchemaType))));
			}
			object obj = method.Invoke(null, new object[1] { schemas });
			if (xmlSchemaProviderAttribute.IsAny)
			{
				if (obj != null)
				{
					throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new InvalidDataContractException(SR.GetString("Method '{0}.{1}()' returns a non-null value. The return value must be null since IsAny=true.", DataContract.GetClrTypeFullName(clrType), methodName)));
				}
				stableName = DataContract.GetDefaultStableName(clrType);
			}
			else if (obj == null)
			{
				xsdType = CreateAnyElementType();
				hasRoot = false;
				stableName = DataContract.GetDefaultStableName(clrType);
			}
			else if (obj is XmlSchemaType { Name: var localName } xmlSchemaType)
			{
				string ns = null;
				if (localName == null || localName.Length == 0)
				{
					DataContract.GetDefaultStableName(DataContract.GetClrTypeFullName(clrType), out localName, out ns);
					stableName = new XmlQualifiedName(localName, ns);
					xmlSchemaType.Annotation = GetSchemaAnnotation(ExportActualType(stableName, new XmlDocument()));
					xsdType = xmlSchemaType;
				}
				else
				{
					foreach (XmlSchema item in schemas.Schemas())
					{
						foreach (XmlSchemaObject item2 in item.Items)
						{
							if (item2 == xmlSchemaType)
							{
								ns = item.TargetNamespace;
								if (ns == null)
								{
									ns = string.Empty;
								}
								break;
							}
						}
						if (ns != null)
						{
							break;
						}
					}
					if (ns == null)
					{
						throw DiagnosticUtility.ExceptionUtility.ThrowHelperError(new InvalidDataContractException(SR.GetString("Schema type '{0}' is missing and required for '{1}' type.", localName, DataContract.GetClrTypeFullName(clrType))));
					}
					stableName = new XmlQualifiedName(localName, ns);
				}
			}
			else
			{
				stableName = (XmlQualifiedName)obj;
			}
		}
		return true;
	}

	private static XmlSchemaComplexType CreateAnyElementType()
	{
		XmlSchemaComplexType obj = new XmlSchemaComplexType
		{
			IsMixed = false,
			Particle = new XmlSchemaSequence()
		};
		XmlSchemaAny item = new XmlSchemaAny
		{
			MinOccurs = 0m,
			ProcessContents = XmlSchemaContentProcessing.Lax
		};
		((XmlSchemaSequence)obj.Particle).Items.Add(item);
		return obj;
	}

	private static XmlSchemaAnnotation GetSchemaAnnotation(params XmlNode[] nodes)
	{
		if (nodes == null || nodes.Length == 0)
		{
			return null;
		}
		bool flag = false;
		for (int i = 0; i < nodes.Length; i++)
		{
			if (nodes[i] != null)
			{
				flag = true;
				break;
			}
		}
		if (!flag)
		{
			return null;
		}
		XmlSchemaAnnotation xmlSchemaAnnotation = new XmlSchemaAnnotation();
		XmlSchemaAppInfo xmlSchemaAppInfo = new XmlSchemaAppInfo();
		xmlSchemaAnnotation.Items.Add(xmlSchemaAppInfo);
		xmlSchemaAppInfo.Markup = nodes;
		return xmlSchemaAnnotation;
	}

	private static XmlSchemaComplexType CreateAnyType()
	{
		XmlSchemaComplexType obj = new XmlSchemaComplexType
		{
			IsMixed = true,
			Particle = new XmlSchemaSequence()
		};
		XmlSchemaAny item = new XmlSchemaAny
		{
			MinOccurs = 0m,
			MaxOccurs = decimal.MaxValue,
			ProcessContents = XmlSchemaContentProcessing.Lax
		};
		((XmlSchemaSequence)obj.Particle).Items.Add(item);
		obj.AnyAttribute = new XmlSchemaAnyAttribute();
		return obj;
	}

	private static XmlElement ExportActualType(XmlQualifiedName typeName, XmlDocument xmlDoc)
	{
		XmlElement xmlElement = xmlDoc.CreateElement(ActualTypeAnnotationName.Name, ActualTypeAnnotationName.Namespace);
		XmlAttribute xmlAttribute = xmlDoc.CreateAttribute("Name");
		xmlAttribute.Value = typeName.Name;
		xmlElement.Attributes.Append(xmlAttribute);
		XmlAttribute xmlAttribute2 = xmlDoc.CreateAttribute("Namespace");
		xmlAttribute2.Value = typeName.Namespace;
		xmlElement.Attributes.Append(xmlAttribute2);
		return xmlElement;
	}
}
