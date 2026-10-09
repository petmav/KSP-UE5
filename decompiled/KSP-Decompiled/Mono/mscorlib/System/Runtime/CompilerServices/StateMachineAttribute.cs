namespace System.Runtime.CompilerServices;

[Serializable]
[AttributeUsage(AttributeTargets.Method, Inherited = false, AllowMultiple = false)]
public class StateMachineAttribute : Attribute
{
	public Type StateMachineType { get; private set; }

	public StateMachineAttribute(Type stateMachineType)
	{
		StateMachineType = stateMachineType;
	}
}
