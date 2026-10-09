using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Xml;

namespace System.Security.Cryptography.Xml;

internal static class SignedXmlDebugLog
{
	internal enum SignedXmlDebugEvent
	{
		BeginCanonicalization,
		BeginCheckSignatureFormat,
		BeginCheckSignedInfo,
		BeginSignatureComputation,
		BeginSignatureVerification,
		CanonicalizedData,
		FormatValidationResult,
		NamespacePropagation,
		ReferenceData,
		SignatureVerificationResult,
		Signing,
		SigningReference,
		VerificationFailure,
		VerifyReference,
		VerifySignedInfo,
		X509Verification,
		UnsafeCanonicalizationMethod,
		UnsafeTransformMethod
	}

	private const string NullString = "(null)";

	private static TraceSource s_traceSource = new TraceSource("System.Security.Cryptography.Xml.SignedXml");

	private static volatile bool s_haveVerboseLogging;

	private static volatile bool s_verboseLogging;

	private static volatile bool s_haveInformationLogging;

	private static volatile bool s_informationLogging;

	private static bool InformationLoggingEnabled
	{
		get
		{
			if (!s_haveInformationLogging)
			{
				s_informationLogging = s_traceSource.Switch.ShouldTrace(TraceEventType.Information);
				s_haveInformationLogging = true;
			}
			return s_informationLogging;
		}
	}

	private static bool VerboseLoggingEnabled
	{
		get
		{
			if (!s_haveVerboseLogging)
			{
				s_verboseLogging = s_traceSource.Switch.ShouldTrace(TraceEventType.Verbose);
				s_haveVerboseLogging = true;
			}
			return s_verboseLogging;
		}
	}

	private static string FormatBytes(byte[] bytes)
	{
		if (bytes == null)
		{
			return "(null)";
		}
		StringBuilder stringBuilder = new StringBuilder(bytes.Length * 2);
		foreach (byte b in bytes)
		{
			stringBuilder.Append(b.ToString("x2", CultureInfo.InvariantCulture));
		}
		return stringBuilder.ToString();
	}

	private static string GetKeyName(object key)
	{
		ICspAsymmetricAlgorithm cspAsymmetricAlgorithm = key as ICspAsymmetricAlgorithm;
		X509Certificate x509Certificate = key as X509Certificate;
		X509Certificate2 x509Certificate2 = key as X509Certificate2;
		string text = null;
		return string.Format(arg1: (cspAsymmetricAlgorithm != null && cspAsymmetricAlgorithm.CspKeyContainerInfo.KeyContainerName != null) ? string.Format(CultureInfo.InvariantCulture, "\"{0}\"", cspAsymmetricAlgorithm.CspKeyContainerInfo.KeyContainerName) : ((x509Certificate2 != null) ? string.Format(CultureInfo.InvariantCulture, "\"{0}\"", x509Certificate2.GetNameInfo(X509NameType.SimpleName, forIssuer: false)) : ((x509Certificate == null) ? key.GetHashCode().ToString("x8", CultureInfo.InvariantCulture) : string.Format(CultureInfo.InvariantCulture, "\"{0}\"", x509Certificate.Subject))), provider: CultureInfo.InvariantCulture, format: "{0}#{1}", arg0: key.GetType().Name);
	}

	private static string GetObjectId(object o)
	{
		return string.Format(CultureInfo.InvariantCulture, "{0}#{1}", o.GetType().Name, o.GetHashCode().ToString("x8", CultureInfo.InvariantCulture));
	}

	private static string GetOidName(Oid oid)
	{
		string text = oid.FriendlyName;
		if (string.IsNullOrEmpty(text))
		{
			text = oid.Value;
		}
		return text;
	}

	internal static void LogBeginCanonicalization(SignedXml signedXml, Transform canonicalizationTransform)
	{
		if (InformationLoggingEnabled)
		{
			string data = string.Format(CultureInfo.InvariantCulture, "Beginning canonicalization using \"{0}\" ({1}).", canonicalizationTransform.Algorithm, canonicalizationTransform.GetType().Name);
			WriteLine(signedXml, TraceEventType.Information, SignedXmlDebugEvent.BeginCanonicalization, data);
		}
		if (VerboseLoggingEnabled)
		{
			string data2 = string.Format(CultureInfo.InvariantCulture, "Canonicalization transform is using resolver {0} and base URI \"{1}\".", canonicalizationTransform.Resolver.GetType(), canonicalizationTransform.BaseURI);
			WriteLine(signedXml, TraceEventType.Verbose, SignedXmlDebugEvent.BeginCanonicalization, data2);
		}
	}

	internal static void LogBeginCheckSignatureFormat(SignedXml signedXml, Func<SignedXml, bool> formatValidator)
	{
		if (InformationLoggingEnabled)
		{
			MethodInfo method = formatValidator.Method;
			string data = string.Format(CultureInfo.InvariantCulture, "Checking signature format using format validator \"[{0}] {1}.{2}\".", method.Module.Assembly.FullName, method.DeclaringType.FullName, method.Name);
			WriteLine(signedXml, TraceEventType.Information, SignedXmlDebugEvent.BeginCheckSignatureFormat, data);
		}
	}

	internal static void LogBeginCheckSignedInfo(SignedXml signedXml, SignedInfo signedInfo)
	{
		if (InformationLoggingEnabled)
		{
			string data = string.Format(CultureInfo.InvariantCulture, "Checking signature on SignedInfo with id \"{0}\".", (signedInfo.Id != null) ? signedInfo.Id : "(null)");
			WriteLine(signedXml, TraceEventType.Information, SignedXmlDebugEvent.BeginCheckSignedInfo, data);
		}
	}

	internal static void LogBeginSignatureComputation(SignedXml signedXml, XmlElement context)
	{
		if (InformationLoggingEnabled)
		{
			WriteLine(signedXml, TraceEventType.Information, SignedXmlDebugEvent.BeginSignatureComputation, "Beginning signature computation.");
		}
		if (VerboseLoggingEnabled)
		{
			string data = string.Format(CultureInfo.InvariantCulture, "Using context: {0}", (context != null) ? context.OuterXml : "(null)");
			WriteLine(signedXml, TraceEventType.Verbose, SignedXmlDebugEvent.BeginSignatureComputation, data);
		}
	}

	internal static void LogBeginSignatureVerification(SignedXml signedXml, XmlElement context)
	{
		if (InformationLoggingEnabled)
		{
			WriteLine(signedXml, TraceEventType.Information, SignedXmlDebugEvent.BeginSignatureVerification, "Beginning signature verification.");
		}
		if (VerboseLoggingEnabled)
		{
			string data = string.Format(CultureInfo.InvariantCulture, "Using context: {0}", (context != null) ? context.OuterXml : "(null)");
			WriteLine(signedXml, TraceEventType.Verbose, SignedXmlDebugEvent.BeginSignatureVerification, data);
		}
	}

	internal static void LogCanonicalizedOutput(SignedXml signedXml, Transform canonicalizationTransform)
	{
		if (VerboseLoggingEnabled)
		{
			using (StreamReader streamReader = new StreamReader(canonicalizationTransform.GetOutput(typeof(Stream)) as Stream))
			{
				string data = string.Format(CultureInfo.InvariantCulture, "Output of canonicalization transform: {0}", streamReader.ReadToEnd());
				WriteLine(signedXml, TraceEventType.Verbose, SignedXmlDebugEvent.CanonicalizedData, data);
			}
		}
	}

	internal static void LogFormatValidationResult(SignedXml signedXml, bool result)
	{
		if (InformationLoggingEnabled)
		{
			string data = (result ? "Signature format validation was successful." : "Signature format validation failed.");
			WriteLine(signedXml, TraceEventType.Information, SignedXmlDebugEvent.FormatValidationResult, data);
		}
	}

	internal static void LogUnsafeCanonicalizationMethod(SignedXml signedXml, string algorithm, IEnumerable<string> validAlgorithms)
	{
		if (!InformationLoggingEnabled)
		{
			return;
		}
		StringBuilder stringBuilder = new StringBuilder();
		foreach (string validAlgorithm in validAlgorithms)
		{
			if (stringBuilder.Length != 0)
			{
				stringBuilder.Append(", ");
			}
			stringBuilder.AppendFormat("\"{0}\"", validAlgorithm);
		}
		string data = string.Format(CultureInfo.InvariantCulture, "Canonicalization method \"{0}\" is not on the safe list. Safe canonicalization methods are: {1}.", algorithm, stringBuilder.ToString());
		WriteLine(signedXml, TraceEventType.Information, SignedXmlDebugEvent.UnsafeCanonicalizationMethod, data);
	}

	internal static void LogUnsafeTransformMethod(SignedXml signedXml, string algorithm, IEnumerable<string> validC14nAlgorithms, IEnumerable<string> validTransformAlgorithms)
	{
		if (!InformationLoggingEnabled)
		{
			return;
		}
		StringBuilder stringBuilder = new StringBuilder();
		foreach (string validC14nAlgorithm in validC14nAlgorithms)
		{
			if (stringBuilder.Length != 0)
			{
				stringBuilder.Append(", ");
			}
			stringBuilder.AppendFormat("\"{0}\"", validC14nAlgorithm);
		}
		foreach (string validTransformAlgorithm in validTransformAlgorithms)
		{
			if (stringBuilder.Length != 0)
			{
				stringBuilder.Append(", ");
			}
			stringBuilder.AppendFormat("\"{0}\"", validTransformAlgorithm);
		}
		string data = string.Format(CultureInfo.InvariantCulture, "Transform method \"{0}\" is not on the safe list. Safe transform methods are: {1}.", algorithm, stringBuilder.ToString());
		WriteLine(signedXml, TraceEventType.Information, SignedXmlDebugEvent.UnsafeTransformMethod, data);
	}

	internal static void LogNamespacePropagation(SignedXml signedXml, XmlNodeList namespaces)
	{
		if (!InformationLoggingEnabled)
		{
			return;
		}
		if (namespaces != null)
		{
			foreach (XmlAttribute @namespace in namespaces)
			{
				string data = string.Format(CultureInfo.InvariantCulture, "Propagating namespace {0}=\"{1}\".", @namespace.Name, @namespace.Value);
				WriteLine(signedXml, TraceEventType.Information, SignedXmlDebugEvent.NamespacePropagation, data);
			}
			return;
		}
		WriteLine(signedXml, TraceEventType.Information, SignedXmlDebugEvent.NamespacePropagation, "No namespaces are being propagated.");
	}

	internal static Stream LogReferenceData(Reference reference, Stream data)
	{
		if (VerboseLoggingEnabled)
		{
			MemoryStream memoryStream = new MemoryStream();
			byte[] array = new byte[4096];
			int num = 0;
			do
			{
				num = data.Read(array, 0, array.Length);
				memoryStream.Write(array, 0, num);
			}
			while (num == array.Length);
			string data2 = string.Format(CultureInfo.InvariantCulture, "Transformed reference contents: {0}", Encoding.UTF8.GetString(memoryStream.ToArray()));
			WriteLine(reference, TraceEventType.Verbose, SignedXmlDebugEvent.ReferenceData, data2);
			memoryStream.Seek(0L, SeekOrigin.Begin);
			return memoryStream;
		}
		return data;
	}

	internal static void LogSigning(SignedXml signedXml, object key, SignatureDescription signatureDescription, HashAlgorithm hash, AsymmetricSignatureFormatter asymmetricSignatureFormatter)
	{
		if (InformationLoggingEnabled)
		{
			string data = string.Format(CultureInfo.InvariantCulture, "Calculating signature with key {0} using signature description {1}, hash algorithm {2}, and asymmetric signature formatter {3}.", GetKeyName(key), signatureDescription.GetType().Name, hash.GetType().Name, asymmetricSignatureFormatter.GetType().Name);
			WriteLine(signedXml, TraceEventType.Information, SignedXmlDebugEvent.Signing, data);
		}
	}

	internal static void LogSigning(SignedXml signedXml, KeyedHashAlgorithm key)
	{
		if (InformationLoggingEnabled)
		{
			string data = string.Format(CultureInfo.InvariantCulture, "Calculating signature using keyed hash algorithm {0}.", key.GetType().Name);
			WriteLine(signedXml, TraceEventType.Information, SignedXmlDebugEvent.Signing, data);
		}
	}

	internal static void LogSigningReference(SignedXml signedXml, Reference reference)
	{
		if (VerboseLoggingEnabled)
		{
			string data = string.Format(CultureInfo.InvariantCulture, "Hashing reference {0}, Uri \"{1}\", Id \"{2}\", Type \"{3}\" with hash algorithm \"{4}\" ({5}).", GetObjectId(reference), reference.Uri, reference.Id, reference.Type, reference.DigestMethod, CryptoHelpers.CreateFromName(reference.DigestMethod).GetType().Name);
			WriteLine(signedXml, TraceEventType.Verbose, SignedXmlDebugEvent.SigningReference, data);
		}
	}

	internal static void LogVerificationFailure(SignedXml signedXml, string failureLocation)
	{
		if (InformationLoggingEnabled)
		{
			string data = string.Format(CultureInfo.InvariantCulture, "Verification failed checking {0}.", failureLocation);
			WriteLine(signedXml, TraceEventType.Information, SignedXmlDebugEvent.VerificationFailure, data);
		}
	}

	internal static void LogVerificationResult(SignedXml signedXml, object key, bool verified)
	{
		if (InformationLoggingEnabled)
		{
			string format = (verified ? "Verification with key {0} was successful." : "Verification with key {0} was not successful.");
			string data = string.Format(CultureInfo.InvariantCulture, format, GetKeyName(key));
			WriteLine(signedXml, TraceEventType.Information, SignedXmlDebugEvent.SignatureVerificationResult, data);
		}
	}

	internal static void LogVerifyKeyUsage(SignedXml signedXml, X509Certificate certificate, X509KeyUsageExtension keyUsages)
	{
		if (InformationLoggingEnabled)
		{
			string data = string.Format(CultureInfo.InvariantCulture, "Found key usages \"{0}\" in extension {1} on certificate {2}.", keyUsages.KeyUsages, GetOidName(keyUsages.Oid), GetKeyName(certificate));
			WriteLine(signedXml, TraceEventType.Verbose, SignedXmlDebugEvent.X509Verification, data);
		}
	}

	internal static void LogVerifyReference(SignedXml signedXml, Reference reference)
	{
		if (InformationLoggingEnabled)
		{
			string data = string.Format(CultureInfo.InvariantCulture, "Processing reference {0}, Uri \"{1}\", Id \"{2}\", Type \"{3}\".", GetObjectId(reference), reference.Uri, reference.Id, reference.Type);
			WriteLine(signedXml, TraceEventType.Verbose, SignedXmlDebugEvent.VerifyReference, data);
		}
	}

	internal static void LogVerifyReferenceHash(SignedXml signedXml, Reference reference, byte[] actualHash, byte[] expectedHash)
	{
		if (VerboseLoggingEnabled)
		{
			string data = string.Format(CultureInfo.InvariantCulture, "Reference {0} hashed with \"{1}\" ({2}) has hash value {3}, expected hash value {4}.", GetObjectId(reference), reference.DigestMethod, CryptoHelpers.CreateFromName(reference.DigestMethod).GetType().Name, FormatBytes(actualHash), FormatBytes(expectedHash));
			WriteLine(signedXml, TraceEventType.Verbose, SignedXmlDebugEvent.VerifyReference, data);
		}
	}

	internal static void LogVerifySignedInfo(SignedXml signedXml, AsymmetricAlgorithm key, SignatureDescription signatureDescription, HashAlgorithm hashAlgorithm, AsymmetricSignatureDeformatter asymmetricSignatureDeformatter, byte[] actualHashValue, byte[] signatureValue)
	{
		if (InformationLoggingEnabled)
		{
			string data = string.Format(CultureInfo.InvariantCulture, "Verifying SignedInfo using key {0}, signature description {1}, hash algorithm {2}, and asymmetric signature deformatter {3}.", GetKeyName(key), signatureDescription.GetType().Name, hashAlgorithm.GetType().Name, asymmetricSignatureDeformatter.GetType().Name);
			WriteLine(signedXml, TraceEventType.Information, SignedXmlDebugEvent.VerifySignedInfo, data);
		}
		if (VerboseLoggingEnabled)
		{
			string data2 = string.Format(CultureInfo.InvariantCulture, "Actual hash value: {0}", FormatBytes(actualHashValue));
			WriteLine(signedXml, TraceEventType.Verbose, SignedXmlDebugEvent.VerifySignedInfo, data2);
			string data3 = string.Format(CultureInfo.InvariantCulture, "Raw signature: {0}", FormatBytes(signatureValue));
			WriteLine(signedXml, TraceEventType.Verbose, SignedXmlDebugEvent.VerifySignedInfo, data3);
		}
	}

	internal static void LogVerifySignedInfo(SignedXml signedXml, KeyedHashAlgorithm mac, byte[] actualHashValue, byte[] signatureValue)
	{
		if (InformationLoggingEnabled)
		{
			string data = string.Format(CultureInfo.InvariantCulture, "Verifying SignedInfo using keyed hash algorithm {0}.", mac.GetType().Name);
			WriteLine(signedXml, TraceEventType.Information, SignedXmlDebugEvent.VerifySignedInfo, data);
		}
		if (VerboseLoggingEnabled)
		{
			string data2 = string.Format(CultureInfo.InvariantCulture, "Actual hash value: {0}", FormatBytes(actualHashValue));
			WriteLine(signedXml, TraceEventType.Verbose, SignedXmlDebugEvent.VerifySignedInfo, data2);
			string data3 = string.Format(CultureInfo.InvariantCulture, "Raw signature: {0}", FormatBytes(signatureValue));
			WriteLine(signedXml, TraceEventType.Verbose, SignedXmlDebugEvent.VerifySignedInfo, data3);
		}
	}

	internal static void LogVerifyX509Chain(SignedXml signedXml, X509Chain chain, X509Certificate certificate)
	{
		if (InformationLoggingEnabled)
		{
			string data = string.Format(CultureInfo.InvariantCulture, "Building and verifying the X509 chain for certificate {0}.", GetKeyName(certificate));
			WriteLine(signedXml, TraceEventType.Information, SignedXmlDebugEvent.X509Verification, data);
		}
		if (VerboseLoggingEnabled)
		{
			string data2 = string.Format(CultureInfo.InvariantCulture, "Revocation mode for chain building: {0}.", chain.ChainPolicy.RevocationFlag);
			WriteLine(signedXml, TraceEventType.Verbose, SignedXmlDebugEvent.X509Verification, data2);
			string data3 = string.Format(CultureInfo.InvariantCulture, "Revocation flag for chain building: {0}.", chain.ChainPolicy.RevocationFlag);
			WriteLine(signedXml, TraceEventType.Verbose, SignedXmlDebugEvent.X509Verification, data3);
			string data4 = string.Format(CultureInfo.InvariantCulture, "Verification flags for chain building: {0}.", chain.ChainPolicy.VerificationFlags);
			WriteLine(signedXml, TraceEventType.Verbose, SignedXmlDebugEvent.X509Verification, data4);
			string data5 = string.Format(CultureInfo.InvariantCulture, "Verification time for chain building: {0}.", chain.ChainPolicy.VerificationTime);
			WriteLine(signedXml, TraceEventType.Verbose, SignedXmlDebugEvent.X509Verification, data5);
			string data6 = string.Format(CultureInfo.InvariantCulture, "URL retrieval timeout for chain building: {0}.", chain.ChainPolicy.UrlRetrievalTimeout);
			WriteLine(signedXml, TraceEventType.Verbose, SignedXmlDebugEvent.X509Verification, data6);
		}
		if (InformationLoggingEnabled)
		{
			X509ChainStatus[] chainStatus = chain.ChainStatus;
			for (int i = 0; i < chainStatus.Length; i++)
			{
				X509ChainStatus x509ChainStatus = chainStatus[i];
				if (x509ChainStatus.Status != 0)
				{
					string data7 = string.Format(CultureInfo.InvariantCulture, "Error building X509 chain: {0}: {1}.", x509ChainStatus.Status, x509ChainStatus.StatusInformation);
					WriteLine(signedXml, TraceEventType.Information, SignedXmlDebugEvent.X509Verification, data7);
				}
			}
		}
		if (VerboseLoggingEnabled)
		{
			StringBuilder stringBuilder = new StringBuilder();
			stringBuilder.Append("Certificate chain:");
			X509ChainElementEnumerator enumerator = chain.ChainElements.GetEnumerator();
			while (enumerator.MoveNext())
			{
				X509ChainElement current = enumerator.Current;
				stringBuilder.AppendFormat(CultureInfo.InvariantCulture, " {0}", GetKeyName(current.Certificate));
			}
			WriteLine(signedXml, TraceEventType.Verbose, SignedXmlDebugEvent.X509Verification, stringBuilder.ToString());
		}
	}

	internal static void LogSignedXmlRecursionLimit(SignedXml signedXml, Reference reference)
	{
		if (InformationLoggingEnabled)
		{
			string data = string.Format(CultureInfo.InvariantCulture, "Signed xml recursion limit hit while trying to decrypt the key. Reference {0} hashed with \"{1}\" and ({2}).", GetObjectId(reference), reference.DigestMethod, CryptoHelpers.CreateFromName(reference.DigestMethod).GetType().Name);
			WriteLine(signedXml, TraceEventType.Information, SignedXmlDebugEvent.VerifySignedInfo, data);
		}
	}

	private static void WriteLine(object source, TraceEventType eventType, SignedXmlDebugEvent eventId, string data)
	{
	}
}
