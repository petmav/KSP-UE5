using System.Collections;
using System.Runtime.InteropServices;
using System.Security.Cryptography.X509Certificates;
using System.Xml;
using Mono.Security.X509;

namespace System.Security.Cryptography.Xml;

public class KeyInfoX509Data : KeyInfoClause
{
	private byte[] x509crl;

	private ArrayList IssuerSerialList;

	private ArrayList SubjectKeyIdList;

	private ArrayList SubjectNameList;

	private ArrayList X509CertificateList;

	public ArrayList Certificates => X509CertificateList;

	public byte[] CRL
	{
		get
		{
			return x509crl;
		}
		set
		{
			x509crl = value;
		}
	}

	public ArrayList IssuerSerials => IssuerSerialList;

	public ArrayList SubjectKeyIds => SubjectKeyIdList;

	public ArrayList SubjectNames => SubjectNameList;

	public KeyInfoX509Data()
	{
	}

	public KeyInfoX509Data(byte[] rgbCert)
	{
		AddCertificate(new System.Security.Cryptography.X509Certificates.X509Certificate(rgbCert));
	}

	public KeyInfoX509Data(System.Security.Cryptography.X509Certificates.X509Certificate cert)
	{
		AddCertificate(cert);
	}

	public KeyInfoX509Data(System.Security.Cryptography.X509Certificates.X509Certificate cert, X509IncludeOption includeOption)
	{
		if (cert == null)
		{
			throw new ArgumentNullException("cert");
		}
		switch (includeOption)
		{
		case X509IncludeOption.None:
		case X509IncludeOption.EndCertOnly:
			AddCertificate(cert);
			break;
		case X509IncludeOption.ExcludeRoot:
			AddCertificatesChainFrom(cert, root: false);
			break;
		case X509IncludeOption.WholeChain:
			AddCertificatesChainFrom(cert, root: true);
			break;
		}
	}

	private void AddCertificatesChainFrom(System.Security.Cryptography.X509Certificates.X509Certificate cert, bool root)
	{
		System.Security.Cryptography.X509Certificates.X509Chain x509Chain = new System.Security.Cryptography.X509Certificates.X509Chain();
		x509Chain.Build(new X509Certificate2(cert));
		X509ChainElementEnumerator enumerator = x509Chain.ChainElements.GetEnumerator();
		while (enumerator.MoveNext())
		{
			byte[] array = enumerator.Current.Certificate.RawData;
			if (!root && new Mono.Security.X509.X509Certificate(array).IsSelfSigned)
			{
				array = null;
			}
			if (array != null)
			{
				AddCertificate(new System.Security.Cryptography.X509Certificates.X509Certificate(array));
			}
		}
	}

	public void AddCertificate(System.Security.Cryptography.X509Certificates.X509Certificate certificate)
	{
		if (certificate == null)
		{
			throw new ArgumentNullException("certificate");
		}
		if (X509CertificateList == null)
		{
			X509CertificateList = new ArrayList();
		}
		X509CertificateList.Add(certificate);
	}

	public void AddIssuerSerial(string issuerName, string serialNumber)
	{
		if (issuerName == null)
		{
			throw new ArgumentException("issuerName");
		}
		if (IssuerSerialList == null)
		{
			IssuerSerialList = new ArrayList();
		}
		X509IssuerSerial x509IssuerSerial = new X509IssuerSerial(issuerName, serialNumber);
		IssuerSerialList.Add(x509IssuerSerial);
	}

	public void AddSubjectKeyId(byte[] subjectKeyId)
	{
		if (SubjectKeyIdList == null)
		{
			SubjectKeyIdList = new ArrayList();
		}
		SubjectKeyIdList.Add(subjectKeyId);
	}

	[ComVisible(false)]
	public void AddSubjectKeyId(string subjectKeyId)
	{
		if (SubjectKeyIdList == null)
		{
			SubjectKeyIdList = new ArrayList();
		}
		byte[] value = null;
		if (subjectKeyId != null)
		{
			value = Convert.FromBase64String(subjectKeyId);
		}
		SubjectKeyIdList.Add(value);
	}

	public void AddSubjectName(string subjectName)
	{
		if (SubjectNameList == null)
		{
			SubjectNameList = new ArrayList();
		}
		SubjectNameList.Add(subjectName);
	}

	public override XmlElement GetXml()
	{
		XmlDocument xmlDocument = new XmlDocument();
		XmlElement xmlElement = xmlDocument.CreateElement("X509Data", "http://www.w3.org/2000/09/xmldsig#");
		xmlElement.SetAttribute("xmlns", "http://www.w3.org/2000/09/xmldsig#");
		if (IssuerSerialList != null && IssuerSerialList.Count > 0)
		{
			foreach (X509IssuerSerial issuerSerial in IssuerSerialList)
			{
				XmlElement xmlElement2 = xmlDocument.CreateElement("X509IssuerSerial", "http://www.w3.org/2000/09/xmldsig#");
				XmlElement xmlElement3 = xmlDocument.CreateElement("X509IssuerName", "http://www.w3.org/2000/09/xmldsig#");
				xmlElement3.InnerText = issuerSerial.IssuerName;
				xmlElement2.AppendChild(xmlElement3);
				XmlElement xmlElement4 = xmlDocument.CreateElement("X509SerialNumber", "http://www.w3.org/2000/09/xmldsig#");
				xmlElement4.InnerText = issuerSerial.SerialNumber;
				xmlElement2.AppendChild(xmlElement4);
				xmlElement.AppendChild(xmlElement2);
			}
		}
		if (SubjectKeyIdList != null && SubjectKeyIdList.Count > 0)
		{
			foreach (byte[] subjectKeyId in SubjectKeyIdList)
			{
				XmlElement xmlElement5 = xmlDocument.CreateElement("X509SKI", "http://www.w3.org/2000/09/xmldsig#");
				xmlElement5.InnerText = Convert.ToBase64String(subjectKeyId);
				xmlElement.AppendChild(xmlElement5);
			}
		}
		if (SubjectNameList != null && SubjectNameList.Count > 0)
		{
			foreach (string subjectName in SubjectNameList)
			{
				XmlElement xmlElement6 = xmlDocument.CreateElement("X509SubjectName", "http://www.w3.org/2000/09/xmldsig#");
				xmlElement6.InnerText = subjectName;
				xmlElement.AppendChild(xmlElement6);
			}
		}
		if (X509CertificateList != null && X509CertificateList.Count > 0)
		{
			foreach (System.Security.Cryptography.X509Certificates.X509Certificate x509Certificate in X509CertificateList)
			{
				XmlElement xmlElement7 = xmlDocument.CreateElement("X509Certificate", "http://www.w3.org/2000/09/xmldsig#");
				xmlElement7.InnerText = Convert.ToBase64String(x509Certificate.GetRawCertData());
				xmlElement.AppendChild(xmlElement7);
			}
		}
		if (x509crl != null)
		{
			XmlElement xmlElement8 = xmlDocument.CreateElement("X509CRL", "http://www.w3.org/2000/09/xmldsig#");
			xmlElement8.InnerText = Convert.ToBase64String(x509crl);
			xmlElement.AppendChild(xmlElement8);
		}
		return xmlElement;
	}

	public override void LoadXml(XmlElement element)
	{
		if (element == null)
		{
			throw new ArgumentNullException("element");
		}
		if (IssuerSerialList != null)
		{
			IssuerSerialList.Clear();
		}
		if (SubjectKeyIdList != null)
		{
			SubjectKeyIdList.Clear();
		}
		if (SubjectNameList != null)
		{
			SubjectNameList.Clear();
		}
		if (X509CertificateList != null)
		{
			X509CertificateList.Clear();
		}
		x509crl = null;
		if (element.LocalName != "X509Data" || element.NamespaceURI != "http://www.w3.org/2000/09/xmldsig#")
		{
			throw new CryptographicException("element");
		}
		XmlElement[] array = null;
		array = XmlSignature.GetChildElements(element, "X509IssuerSerial");
		if (array != null)
		{
			foreach (XmlElement xel in array)
			{
				XmlElement childElement = XmlSignature.GetChildElement(xel, "X509IssuerName", "http://www.w3.org/2000/09/xmldsig#");
				XmlElement childElement2 = XmlSignature.GetChildElement(xel, "X509SerialNumber", "http://www.w3.org/2000/09/xmldsig#");
				AddIssuerSerial(childElement.InnerText, childElement2.InnerText);
			}
		}
		array = XmlSignature.GetChildElements(element, "X509SKI");
		if (array != null)
		{
			for (int j = 0; j < array.Length; j++)
			{
				byte[] subjectKeyId = Convert.FromBase64String(array[j].InnerXml);
				AddSubjectKeyId(subjectKeyId);
			}
		}
		array = XmlSignature.GetChildElements(element, "X509SubjectName");
		if (array != null)
		{
			for (int k = 0; k < array.Length; k++)
			{
				AddSubjectName(array[k].InnerXml);
			}
		}
		array = XmlSignature.GetChildElements(element, "X509Certificate");
		if (array != null)
		{
			for (int l = 0; l < array.Length; l++)
			{
				byte[] data = Convert.FromBase64String(array[l].InnerXml);
				AddCertificate(new System.Security.Cryptography.X509Certificates.X509Certificate(data));
			}
		}
		XmlElement childElement3 = XmlSignature.GetChildElement(element, "X509CRL", "http://www.w3.org/2000/09/xmldsig#");
		if (childElement3 != null)
		{
			x509crl = Convert.FromBase64String(childElement3.InnerXml);
		}
	}
}
