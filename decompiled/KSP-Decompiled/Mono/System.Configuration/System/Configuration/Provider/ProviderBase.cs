using System.Collections.Specialized;

namespace System.Configuration.Provider;

public abstract class ProviderBase
{
	private bool alreadyInitialized;

	private string _description;

	private string _name;

	public virtual string Name => _name;

	public virtual string Description => _description;

	public virtual void Initialize(string name, NameValueCollection config)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		if (name.Length == 0)
		{
			throw new ArgumentException("Provider name cannot be null or empty.", "name");
		}
		if (alreadyInitialized)
		{
			throw new InvalidOperationException("This provider instance has already been initialized.");
		}
		alreadyInitialized = true;
		_name = name;
		if (config != null)
		{
			_description = config["description"];
			config.Remove("description");
		}
		if (string.IsNullOrEmpty(_description))
		{
			_description = _name;
		}
	}
}
