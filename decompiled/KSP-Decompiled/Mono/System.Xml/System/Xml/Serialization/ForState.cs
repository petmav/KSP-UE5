using System.Reflection.Emit;

namespace System.Xml.Serialization;

internal class ForState
{
	private LocalBuilder indexVar;

	private Label beginLabel;

	private Label testLabel;

	private object end;

	internal LocalBuilder Index => indexVar;

	internal Label BeginLabel => beginLabel;

	internal Label TestLabel => testLabel;

	internal object End => end;

	internal ForState(LocalBuilder indexVar, Label beginLabel, Label testLabel, object end)
	{
		this.indexVar = indexVar;
		this.beginLabel = beginLabel;
		this.testLabel = testLabel;
		this.end = end;
	}
}
