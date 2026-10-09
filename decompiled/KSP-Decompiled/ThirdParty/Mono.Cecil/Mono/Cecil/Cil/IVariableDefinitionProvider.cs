using Mono.Collections.Generic;

namespace Mono.Cecil.Cil
{
	public interface IVariableDefinitionProvider
	{
		bool HasVariables { get; }

		Collection<VariableDefinition> Variables { get; }
	}
}
