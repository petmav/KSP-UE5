using System.Diagnostics;

namespace System.Runtime.Versioning;

[Conditional("RESOURCE_ANNOTATION_WORK")]
[AttributeUsage(AttributeTargets.Constructor | AttributeTargets.Method | AttributeTargets.Property, Inherited = false)]
public sealed class ResourceConsumptionAttribute : Attribute
{
	private ResourceScope _consumptionScope;

	private ResourceScope _resourceScope;

	public ResourceScope ResourceScope => _resourceScope;

	public ResourceScope ConsumptionScope => _consumptionScope;

	public ResourceConsumptionAttribute(ResourceScope resourceScope)
	{
		_resourceScope = resourceScope;
		_consumptionScope = _resourceScope;
	}

	public ResourceConsumptionAttribute(ResourceScope resourceScope, ResourceScope consumptionScope)
	{
		_resourceScope = resourceScope;
		_consumptionScope = consumptionScope;
	}
}
