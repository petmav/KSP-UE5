namespace System.Threading;

internal static class LazyHelpers<T>
{
	internal static Func<T> s_activatorFactorySelector = ActivatorFactorySelector;

	private static T ActivatorFactorySelector()
	{
		try
		{
			return (T)Activator.CreateInstance(typeof(T));
		}
		catch (MissingMethodException)
		{
			throw new MissingMemberException(Environment.GetResourceString("The lazily-initialized type does not have a public, parameterless constructor."));
		}
	}
}
