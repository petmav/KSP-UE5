using System;
using System.Reflection;
using UnityEngine;

namespace Expansions.Missions.Editor
{
	public class BaseAPField : BaseField<MEGUI_Control>
	{
		[SerializeField]
		private bool _gapDisplay;

		[SerializeField]
		private string _group;

		[SerializeField]
		private string _groupDisplayName;

		private MethodInfo _onValueChange;

		private MethodInfo _onControlCreated;

		private MethodInfo _onControlSetupComplete;

		private MethodInfo _compareValuesForCheckpoint;

		private PropertyInfo _propertyInfo;

		private bool _hideWhenSiblingsExist;

		private bool _hideWhenStartNode;

		private bool _hideWhenDocked;

		private bool _hideWhenInputConnected;

		private bool _hideWhenOutputConnected;

		private bool _hideWhenNoTestModules;

		private bool _hideWhenNoActionModules;

		private bool _hideOnSetup;

		[SerializeField]
		private int _order;

		private bool _tabStop;

		[SerializeField]
		private bool _groupStartCollapsed;

		public bool gapDisplay
		{
			get
			{
				return _gapDisplay;
			}
			protected set
			{
				_gapDisplay = value;
			}
		}

		public MethodInfo OnValueChange => _onValueChange;

		public MethodInfo OnControlCreated => _onControlCreated;

		public MethodInfo OnControlSetupComplete => _onControlSetupComplete;

		public MethodInfo CompareValuesForCheckpoint => _compareValuesForCheckpoint;

		public PropertyInfo PropertyInfo
		{
			get
			{
				return _propertyInfo;
			}
			protected set
			{
				_propertyInfo = value;
			}
		}

		public string FieldID => string.Concat(base.MemberInfo.DeclaringType, ".", base.name);

		public Type FieldType
		{
			get
			{
				if (_propertyInfo != null)
				{
					return _propertyInfo.PropertyType;
				}
				if (base.FieldInfo != null)
				{
					return base.FieldInfo.FieldType;
				}
				return null;
			}
		}

		public string Group
		{
			get
			{
				return _group;
			}
			protected set
			{
				_group = value;
			}
		}

		public string GroupDisplayName
		{
			get
			{
				return _groupDisplayName;
			}
			protected set
			{
				_groupDisplayName = value;
			}
		}

		public bool HideWhenSiblingsExist
		{
			get
			{
				return _hideWhenSiblingsExist;
			}
			protected set
			{
				_hideWhenSiblingsExist = value;
			}
		}

		public bool HideWhenStartNode
		{
			get
			{
				return _hideWhenStartNode;
			}
			protected set
			{
				_hideWhenStartNode = value;
			}
		}

		public bool HideWhenDocked
		{
			get
			{
				return _hideWhenDocked;
			}
			protected set
			{
				_hideWhenDocked = value;
			}
		}

		public bool HideWhenInputConnected
		{
			get
			{
				return _hideWhenInputConnected;
			}
			protected set
			{
				_hideWhenInputConnected = value;
			}
		}

		public bool HideWhenOutputConnected
		{
			get
			{
				return _hideWhenOutputConnected;
			}
			protected set
			{
				_hideWhenOutputConnected = value;
			}
		}

		public bool HideWhenNoTestModules
		{
			get
			{
				return _hideWhenNoTestModules;
			}
			protected set
			{
				_hideWhenNoTestModules = value;
			}
		}

		public bool HideWhenNoActionModules
		{
			get
			{
				return _hideWhenNoActionModules;
			}
			protected set
			{
				_hideWhenNoActionModules = value;
			}
		}

		public bool HideOnSetup
		{
			get
			{
				return _hideOnSetup;
			}
			protected set
			{
				_hideOnSetup = value;
			}
		}

		public int Order
		{
			get
			{
				return _order;
			}
			protected set
			{
				_order = value;
			}
		}

		public bool TabStop
		{
			get
			{
				return _tabStop;
			}
			protected set
			{
				_tabStop = value;
			}
		}

		public bool GroupStartCollapsed
		{
			get
			{
				return _groupStartCollapsed;
			}
			protected set
			{
				_groupStartCollapsed = value;
			}
		}

		public BaseAPField(MEGUI_Control fieldAttrib, MemberInfo fieldInfo, object host)
			: base(fieldAttrib, fieldInfo, host)
		{
			_gapDisplay = fieldAttrib.gapDisplay;
			_group = fieldAttrib.group;
			_groupDisplayName = fieldAttrib.groupDisplayName;
			_propertyInfo = fieldInfo as PropertyInfo;
			if (!string.IsNullOrEmpty(fieldAttrib.onValueChange))
			{
				_onValueChange = host.GetType().GetMethod(fieldAttrib.onValueChange, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic, null, new Type[1] { FieldType }, null);
			}
			if (!string.IsNullOrEmpty(fieldAttrib.onControlCreated))
			{
				_onControlCreated = host.GetType().GetMethod(fieldAttrib.onControlCreated, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
			}
			if (!string.IsNullOrEmpty(fieldAttrib.onControlSetupComplete))
			{
				_onControlSetupComplete = host.GetType().GetMethod(fieldAttrib.onControlSetupComplete, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
			}
			if (!string.IsNullOrEmpty(fieldAttrib.compareValuesForCheckpoint))
			{
				_compareValuesForCheckpoint = host.GetType().GetMethod(fieldAttrib.compareValuesForCheckpoint, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic, null, new Type[1] { FieldType }, null);
				if (_compareValuesForCheckpoint != null && _compareValuesForCheckpoint.ReturnType != typeof(bool))
				{
					_compareValuesForCheckpoint = null;
					Debug.Log("[Checkpoint validation]: The method " + fieldAttrib.compareValuesForCheckpoint + " has an invalid return type.");
				}
			}
			_hideWhenSiblingsExist = fieldAttrib.hideWhenSiblingsExist;
			_hideWhenStartNode = fieldAttrib.hideWhenStartNode;
			_hideWhenDocked = fieldAttrib.hideWhenDocked;
			_hideWhenInputConnected = fieldAttrib.hideWhenInputConnected;
			_hideWhenOutputConnected = fieldAttrib.hideWhenOutputConnected;
			_hideWhenNoTestModules = fieldAttrib.hideWhenNoTestModules;
			_hideWhenNoActionModules = fieldAttrib.hideWhenNoActionModules;
			_hideOnSetup = fieldAttrib.hideOnSetup;
			_order = fieldAttrib.order;
			_tabStop = fieldAttrib.tabStop;
			_groupStartCollapsed = fieldAttrib.groupStartCollapsed;
		}

		public bool SetValue(object newValue)
		{
			if (base.MemberInfo.MemberType == MemberTypes.Field)
			{
				if (SetValue(newValue, base.host))
				{
					if (_onValueChange != null)
					{
						_onValueChange.Invoke(base.host, new object[1] { newValue });
					}
					MissionEditorValidator.RunValidationOnParamChange();
					return true;
				}
				return false;
			}
			try
			{
				_propertyInfo.SetValue(base.host, newValue, null);
				if (_onValueChange != null)
				{
					_onValueChange.Invoke(base.host, new object[1] { newValue });
				}
				MissionEditorValidator.RunValidationOnParamChange();
				return true;
			}
			catch (Exception ex)
			{
				PDebug.Error(string.Concat("Value '", newValue, "' could not be set to field '", base.name, "'"));
				PDebug.Error(ex.Message + "\n" + ex.StackTrace + "\n" + ex.Data);
				return false;
			}
		}

		public object GetValue()
		{
			if (base.MemberInfo.MemberType == MemberTypes.Field)
			{
				return GetValue(base.host);
			}
			try
			{
				return _propertyInfo.GetValue(base.host, null);
			}
			catch
			{
				PDebug.Error("Value could not be retrieved from field '" + base.name + "'");
				return null;
			}
		}

		public T GetValue<T>()
		{
			return (T)GetValue();
		}

		public override void SetOriginalValue()
		{
			if (base.MemberInfo.MemberType == MemberTypes.Field)
			{
				base.SetOriginalValue();
			}
			else
			{
				base.originalValue = _propertyInfo.GetValue(base.host, null);
			}
		}
	}
}
