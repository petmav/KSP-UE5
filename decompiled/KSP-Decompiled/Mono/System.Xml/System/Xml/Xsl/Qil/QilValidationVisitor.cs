using System.Diagnostics;
using System.Xml.Utils;

namespace System.Xml.Xsl.Qil;

internal class QilValidationVisitor : QilScopedVisitor
{
	private SubstitutionList subs = new SubstitutionList();

	private QilTypeChecker typeCheck = new QilTypeChecker();

	[Conditional("DEBUG")]
	public static void Validate(QilNode node)
	{
		new QilValidationVisitor().VisitAssumeReference(node);
	}

	protected QilValidationVisitor()
	{
	}

	[Conditional("DEBUG")]
	internal static void SetError(QilNode n, string message)
	{
		message = System.Xml.Utils.Res.GetString("QIL Validation Error! '{0}'.", message);
		if (n.Annotation is string text)
		{
			message = text + "\n" + message;
		}
		n.Annotation = message;
	}
}
