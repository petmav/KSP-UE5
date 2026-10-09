using System.Globalization;

namespace System.Xml.Linq;

internal static class Res
{
	internal const string Argument_AddAttribute = "Argument_AddAttribute";

	internal const string Argument_AddNode = "Argument_AddNode";

	internal const string Argument_AddNonWhitespace = "Argument_AddNonWhitespace";

	internal const string Argument_ConvertToString = "Argument_ConvertToString";

	internal const string Argument_CreateNavigator = "Argument_CreateNavigator";

	internal const string Argument_InvalidExpandedName = "Argument_InvalidExpandedName";

	internal const string Argument_InvalidPIName = "Argument_InvalidPIName";

	internal const string Argument_InvalidPrefix = "Argument_InvalidPrefix";

	internal const string Argument_MustBeDerivedFrom = "Argument_MustBeDerivedFrom";

	internal const string Argument_NamespaceDeclarationPrefixed = "Argument_NamespaceDeclarationPrefixed";

	internal const string Argument_NamespaceDeclarationXml = "Argument_NamespaceDeclarationXml";

	internal const string Argument_NamespaceDeclarationXmlns = "Argument_NamespaceDeclarationXmlns";

	internal const string Argument_XObjectValue = "Argument_XObjectValue";

	internal const string InvalidOperation_BadNodeType = "InvalidOperation_BadNodeType";

	internal const string InvalidOperation_DocumentStructure = "InvalidOperation_DocumentStructure";

	internal const string InvalidOperation_DuplicateAttribute = "InvalidOperation_DuplicateAttribute";

	internal const string InvalidOperation_ExpectedEndOfFile = "InvalidOperation_ExpectedEndOfFile";

	internal const string InvalidOperation_ExpectedInteractive = "InvalidOperation_ExpectedInteractive";

	internal const string InvalidOperation_ExpectedNodeType = "InvalidOperation_ExpectedNodeType";

	internal const string InvalidOperation_ExternalCode = "InvalidOperation_ExternalCode";

	internal const string InvalidOperation_DeserializeInstance = "InvalidOperation_DeserializeInstance";

	internal const string InvalidOperation_MissingAncestor = "InvalidOperation_MissingAncestor";

	internal const string InvalidOperation_MissingParent = "InvalidOperation_MissingParent";

	internal const string InvalidOperation_MissingRoot = "InvalidOperation_MissingRoot";

	internal const string InvalidOperation_UnexpectedEvaluation = "InvalidOperation_UnexpectedEvaluation";

	internal const string InvalidOperation_UnexpectedNodeType = "InvalidOperation_UnexpectedNodeType";

	internal const string InvalidOperation_UnresolvedEntityReference = "InvalidOperation_UnresolvedEntityReference";

	internal const string InvalidOperation_WriteAttribute = "InvalidOperation_WriteAttribute";

	internal const string NotSupported_CheckValidity = "NotSupported_CheckValidity";

	internal const string NotSupported_MoveToId = "NotSupported_MoveToId";

	internal const string NotSupported_WriteBase64 = "NotSupported_WriteBase64";

	internal const string NotSupported_WriteEntityRef = "NotSupported_WriteEntityRef";

	public static string GetString(string name)
	{
		return name switch
		{
			"Argument_AddAttribute" => "An attribute cannot be added to content.", 
			"Argument_AddNode" => "A node of type {0} cannot be added to content.", 
			"Argument_AddNonWhitespace" => "Non white space characters cannot be added to content.", 
			"Argument_ConvertToString" => "The argument cannot be converted to a string.", 
			"Argument_CreateNavigator" => "This XPathNavigator cannot be created on a node of type {0}.", 
			"Argument_InvalidExpandedName" => "'{0}' is an invalid expanded name.", 
			"Argument_InvalidPIName" => "'{0}' is an invalid name for a processing instruction.", 
			"Argument_InvalidPrefix" => "'{0}' is an invalid prefix.", 
			"Argument_MustBeDerivedFrom" => "The argument must be derived from {0}.", 
			"Argument_NamespaceDeclarationPrefixed" => "The prefix '{0}' cannot be bound to the empty namespace name.", 
			"Argument_NamespaceDeclarationXml" => "The prefix 'xml' is bound to the namespace name 'http://www.w3.org/XML/1998/namespace'. Other prefixes must not be bound to this namespace name, and it must not be declared as the default namespace.", 
			"Argument_NamespaceDeclarationXmlns" => "The prefix 'xmlns' is bound to the namespace name 'http://www.w3.org/2000/xmlns/'. It must not be declared. Other prefixes must not be bound to this namespace name, and it must not be declared as the default namespace.", 
			"Argument_XObjectValue" => "An XObject cannot be used as a value.", 
			"InvalidOperation_BadNodeType" => "This operation is not valid on a node of type {0}.", 
			"InvalidOperation_DocumentStructure" => "This operation would create an incorrectly structured document.", 
			"InvalidOperation_DuplicateAttribute" => "Duplicate attribute.", 
			"InvalidOperation_ExpectedEndOfFile" => "The XmlReader state should be EndOfFile after this operation.", 
			"InvalidOperation_ExpectedInteractive" => "The XmlReader state should be Interactive.", 
			"InvalidOperation_ExpectedNodeType" => "The XmlReader must be on a node of type {0} instead of a node of type {1}.", 
			"InvalidOperation_ExternalCode" => "This operation was corrupted by external code.", 
			"InvalidOperation_DeserializeInstance" => "This instance cannot be deserialized.", 
			"InvalidOperation_MissingAncestor" => "A common ancestor is missing.", 
			"InvalidOperation_MissingParent" => "The parent is missing.", 
			"InvalidOperation_MissingRoot" => "The root element is missing.", 
			"InvalidOperation_UnexpectedEvaluation" => "The XPath expression evaluated to unexpected type {0}.", 
			"InvalidOperation_UnexpectedNodeType" => "The XmlReader should not be on a node of type {0}.", 
			"InvalidOperation_UnresolvedEntityReference" => "The XmlReader cannot resolve entity references.", 
			"InvalidOperation_WriteAttribute" => "An attribute cannot be written after content.", 
			"NotSupported_CheckValidity" => "This XPathNavigator does not support XSD validation.", 
			"NotSupported_MoveToId" => "This XPathNavigator does not support IDs.", 
			"NotSupported_WriteBase64" => "This XmlWriter does not support base64 encoded data.", 
			"NotSupported_WriteEntityRef" => "This XmlWriter does not support entity references.", 
			_ => null, 
		};
	}

	public static string GetString(string name, params object[] args)
	{
		string @string = GetString(name);
		if (args == null || args.Length == 0)
		{
			return @string;
		}
		return string.Format(CultureInfo.CurrentCulture, @string, args);
	}
}
