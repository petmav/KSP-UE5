namespace System.Xml.Serialization;

internal class ImportStructWorkItem
{
	private StructModel model;

	private StructMapping mapping;

	internal StructModel Model => model;

	internal StructMapping Mapping => mapping;

	internal ImportStructWorkItem(StructModel model, StructMapping mapping)
	{
		this.model = model;
		this.mapping = mapping;
	}
}
