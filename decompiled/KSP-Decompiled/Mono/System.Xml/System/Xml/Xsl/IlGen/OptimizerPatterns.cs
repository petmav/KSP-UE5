using System.Xml.Xsl.Qil;

namespace System.Xml.Xsl.IlGen;

internal class OptimizerPatterns : IQilAnnotation
{
	private static readonly int PatternCount = Enum.GetValues(typeof(OptimizerPatternName)).Length;

	private int patterns;

	private bool isReadOnly;

	private object arg0;

	private object arg1;

	private object arg2;

	private static volatile OptimizerPatterns ZeroOrOneDefault;

	private static volatile OptimizerPatterns MaybeManyDefault;

	private static volatile OptimizerPatterns DodDefault;

	public virtual string Name => "Patterns";

	public static OptimizerPatterns Read(QilNode nd)
	{
		OptimizerPatterns optimizerPatterns = ((nd.Annotation is XmlILAnnotation xmlILAnnotation) ? xmlILAnnotation.Patterns : null);
		if (optimizerPatterns == null)
		{
			if (!nd.XmlType.MaybeMany)
			{
				if (ZeroOrOneDefault == null)
				{
					optimizerPatterns = new OptimizerPatterns();
					optimizerPatterns.AddPattern(OptimizerPatternName.IsDocOrderDistinct);
					optimizerPatterns.AddPattern(OptimizerPatternName.SameDepth);
					optimizerPatterns.isReadOnly = true;
					ZeroOrOneDefault = optimizerPatterns;
				}
				else
				{
					optimizerPatterns = ZeroOrOneDefault;
				}
			}
			else if (nd.XmlType.IsDod)
			{
				if (DodDefault == null)
				{
					optimizerPatterns = new OptimizerPatterns();
					optimizerPatterns.AddPattern(OptimizerPatternName.IsDocOrderDistinct);
					optimizerPatterns.isReadOnly = true;
					DodDefault = optimizerPatterns;
				}
				else
				{
					optimizerPatterns = DodDefault;
				}
			}
			else if (MaybeManyDefault == null)
			{
				optimizerPatterns = new OptimizerPatterns();
				optimizerPatterns.isReadOnly = true;
				MaybeManyDefault = optimizerPatterns;
			}
			else
			{
				optimizerPatterns = MaybeManyDefault;
			}
		}
		return optimizerPatterns;
	}

	public static OptimizerPatterns Write(QilNode nd)
	{
		XmlILAnnotation xmlILAnnotation = XmlILAnnotation.Write(nd);
		OptimizerPatterns optimizerPatterns = xmlILAnnotation.Patterns;
		if (optimizerPatterns == null || optimizerPatterns.isReadOnly)
		{
			optimizerPatterns = (xmlILAnnotation.Patterns = new OptimizerPatterns());
			if (!nd.XmlType.MaybeMany)
			{
				optimizerPatterns.AddPattern(OptimizerPatternName.IsDocOrderDistinct);
				optimizerPatterns.AddPattern(OptimizerPatternName.SameDepth);
			}
			else if (nd.XmlType.IsDod)
			{
				optimizerPatterns.AddPattern(OptimizerPatternName.IsDocOrderDistinct);
			}
		}
		return optimizerPatterns;
	}

	public static void Inherit(QilNode ndSrc, QilNode ndDst, OptimizerPatternName pattern)
	{
		OptimizerPatterns optimizerPatterns = Read(ndSrc);
		if (optimizerPatterns.MatchesPattern(pattern))
		{
			OptimizerPatterns optimizerPatterns2 = Write(ndDst);
			optimizerPatterns2.AddPattern(pattern);
			switch (pattern)
			{
			case OptimizerPatternName.Step:
				optimizerPatterns2.AddArgument(OptimizerPatternArgument.StepNode, optimizerPatterns.GetArgument(OptimizerPatternArgument.StepNode));
				optimizerPatterns2.AddArgument(OptimizerPatternArgument.StepInput, optimizerPatterns.GetArgument(OptimizerPatternArgument.StepInput));
				break;
			case OptimizerPatternName.FilterElements:
				optimizerPatterns2.AddArgument(OptimizerPatternArgument.ElementQName, optimizerPatterns.GetArgument(OptimizerPatternArgument.ElementQName));
				break;
			case OptimizerPatternName.FilterContentKind:
				optimizerPatterns2.AddArgument(OptimizerPatternArgument.ElementQName, optimizerPatterns.GetArgument(OptimizerPatternArgument.ElementQName));
				break;
			case OptimizerPatternName.EqualityIndex:
				optimizerPatterns2.AddArgument(OptimizerPatternArgument.StepNode, optimizerPatterns.GetArgument(OptimizerPatternArgument.StepNode));
				optimizerPatterns2.AddArgument(OptimizerPatternArgument.StepInput, optimizerPatterns.GetArgument(OptimizerPatternArgument.StepInput));
				break;
			case OptimizerPatternName.DodReverse:
			case OptimizerPatternName.JoinAndDod:
				optimizerPatterns2.AddArgument(OptimizerPatternArgument.ElementQName, optimizerPatterns.GetArgument(OptimizerPatternArgument.ElementQName));
				break;
			case OptimizerPatternName.MaxPosition:
				optimizerPatterns2.AddArgument(OptimizerPatternArgument.ElementQName, optimizerPatterns.GetArgument(OptimizerPatternArgument.ElementQName));
				break;
			case OptimizerPatternName.SingleTextRtf:
				optimizerPatterns2.AddArgument(OptimizerPatternArgument.ElementQName, optimizerPatterns.GetArgument(OptimizerPatternArgument.ElementQName));
				break;
			case OptimizerPatternName.FilterAttributeKind:
			case OptimizerPatternName.IsDocOrderDistinct:
			case OptimizerPatternName.IsPositional:
			case OptimizerPatternName.SameDepth:
				break;
			}
		}
	}

	public void AddArgument(OptimizerPatternArgument argId, object arg)
	{
		switch ((int)argId)
		{
		case 0:
			arg0 = arg;
			break;
		case 1:
			arg1 = arg;
			break;
		case 2:
			arg2 = arg;
			break;
		}
	}

	public object GetArgument(OptimizerPatternArgument argNum)
	{
		object result = null;
		switch ((int)argNum)
		{
		case 0:
			result = arg0;
			break;
		case 1:
			result = arg1;
			break;
		case 2:
			result = arg2;
			break;
		}
		return result;
	}

	public void AddPattern(OptimizerPatternName pattern)
	{
		patterns |= 1 << (int)pattern;
	}

	public bool MatchesPattern(OptimizerPatternName pattern)
	{
		return (patterns & (1 << (int)pattern)) != 0;
	}

	public override string ToString()
	{
		string text = "";
		for (int i = 0; i < PatternCount; i++)
		{
			if (MatchesPattern((OptimizerPatternName)i))
			{
				if (text.Length != 0)
				{
					text += ", ";
				}
				string text2 = text;
				OptimizerPatternName optimizerPatternName = (OptimizerPatternName)i;
				text = text2 + optimizerPatternName;
			}
		}
		return text;
	}
}
