using System;
using System.Collections.Generic;
using System.Reflection;
using KSP.Localization;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	public abstract class AppUIMember : MonoBehaviour
	{
		protected AppUI_Data _host;

		protected AppUI_Control _attribs;

		protected FieldInfo _field;

		protected PropertyInfo _property;

		protected MemberInfo _member;

		protected bool isEnum;

		protected Type valueType;

		public TextMeshProUGUI guiNameLabel;

		protected RectTransform guiNameLabelRectTransform;

		[SerializeField]
		protected RectTransform valueHolderRectTransform;

		protected Vector2 valueRectOffsetMin;

		protected AppUIInputPanel parentPanel;

		protected VerticalLayoutGroup parentLayoutGroup;

		[SerializeField]
		protected List<UIHoverText> hoverTextList;

		[SerializeField]
		private string _hoverText = "";

		private AppUIMemberLabelList labelList;

		public bool showGuiName = true;

		[SerializeField]
		private string _guiName;

		public AppUI_Control.HorizontalAlignment guiNameHorizAlignment = AppUI_Control.HorizontalAlignment.Left;

		public AppUI_Control.VerticalAlignment guiNameVertAlignment = AppUI_Control.VerticalAlignment.Midline;

		public int order;

		public bool hideOnError;

		public string HoverText
		{
			get
			{
				return _hoverText;
			}
			set
			{
				_hoverText = value;
				if (hoverTextList != null)
				{
					for (int i = 0; i < hoverTextList.Count; i++)
					{
						hoverTextList[i].hoverText = _hoverText;
					}
				}
			}
		}

		public string guiName
		{
			get
			{
				return _guiName;
			}
			set
			{
				_guiName = Localizer.Format(value);
			}
		}

		public bool IsInitialized { get; private set; }

		public string Name
		{
			get
			{
				if (_member != null)
				{
					return _member.Name;
				}
				return "";
			}
		}

		protected virtual void OnInitialized()
		{
		}

		protected abstract void OnRefreshUI();

		protected virtual void OnAwake()
		{
		}

		protected virtual void OnStart()
		{
		}

		protected virtual void OnUpdate()
		{
		}

		internal virtual bool AnyTextFieldHasFocus()
		{
			return false;
		}

		public virtual void Setup(AppUI_Data host, MemberInfo member, AppUI_Control attribs, AppUIInputPanel parentPanel)
		{
			_host = host;
			_member = member;
			_field = _member as FieldInfo;
			_property = _member as PropertyInfo;
			_attribs = attribs;
			this.parentPanel = parentPanel;
			parentLayoutGroup = parentPanel.GetComponent<VerticalLayoutGroup>();
			if ((_field == null) & (_property == null))
			{
				Debug.LogError($"[AppUIMember]: Unable to setup {member.Name} on {host} - not a field or property");
				return;
			}
			if (_field != null)
			{
				isEnum = _field.FieldType.BaseType == typeof(Enum);
				valueType = _field.FieldType;
			}
			else if (_property != null)
			{
				isEnum = _property.PropertyType.BaseType == typeof(Enum);
				valueType = _property.PropertyType;
			}
			guiName = (string.IsNullOrEmpty(attribs.guiName) ? Name : attribs.guiName);
			order = attribs.order;
			showGuiName = attribs.showGuiName;
			guiNameVertAlignment = attribs.guiNameVertAlignment;
			guiNameHorizAlignment = attribs.guiNameHorizAlignment;
			hideOnError = attribs.hideOnError;
			if (guiNameLabel != null)
			{
				guiNameLabelRectTransform = guiNameLabel.transform as RectTransform;
			}
			HoverText = (string.IsNullOrEmpty(attribs.hoverText) ? "" : attribs.hoverText);
			HoverText = (string.IsNullOrEmpty(attribs.hoverText) ? "" : attribs.hoverText);
			labelList = this as AppUIMemberLabelList;
			IsInitialized = true;
			OnInitialized();
			RefreshUI();
		}

		public void RefreshUI()
		{
			if (valueHolderRectTransform != null)
			{
				valueRectOffsetMin = valueHolderRectTransform.offsetMin;
				valueRectOffsetMin.x = (showGuiName ? guiNameLabelRectTransform.sizeDelta.x : 0f);
				valueHolderRectTransform.offsetMin = valueRectOffsetMin;
			}
			if (guiNameLabel != null)
			{
				guiNameLabel.text = guiName;
				guiNameLabel.gameObject.SetActive(showGuiName);
			}
			OnRefreshUI();
			if (labelList != null && guiNameLabel != null)
			{
				guiNameLabel.text = labelList.newGuiName;
			}
			switch (guiNameHorizAlignment)
			{
			case AppUI_Control.HorizontalAlignment.Left:
				switch (guiNameVertAlignment)
				{
				case AppUI_Control.VerticalAlignment.Top:
					guiNameLabel.alignment = TextAlignmentOptions.TopLeft;
					break;
				case AppUI_Control.VerticalAlignment.Bottom:
					guiNameLabel.alignment = TextAlignmentOptions.BottomLeft;
					break;
				case AppUI_Control.VerticalAlignment.Midline:
					guiNameLabel.alignment = TextAlignmentOptions.MidlineLeft;
					break;
				case AppUI_Control.VerticalAlignment.Capline:
					guiNameLabel.alignment = TextAlignmentOptions.CaplineLeft;
					break;
				}
				break;
			case AppUI_Control.HorizontalAlignment.Middle:
				switch (guiNameVertAlignment)
				{
				case AppUI_Control.VerticalAlignment.Top:
					guiNameLabel.alignment = TextAlignmentOptions.Top;
					break;
				case AppUI_Control.VerticalAlignment.Bottom:
					guiNameLabel.alignment = TextAlignmentOptions.Bottom;
					break;
				case AppUI_Control.VerticalAlignment.Midline:
					guiNameLabel.alignment = TextAlignmentOptions.Midline;
					break;
				case AppUI_Control.VerticalAlignment.Capline:
					guiNameLabel.alignment = TextAlignmentOptions.Capline;
					break;
				}
				break;
			case AppUI_Control.HorizontalAlignment.Right:
				switch (guiNameVertAlignment)
				{
				case AppUI_Control.VerticalAlignment.Top:
					guiNameLabel.alignment = TextAlignmentOptions.TopRight;
					break;
				case AppUI_Control.VerticalAlignment.Bottom:
					guiNameLabel.alignment = TextAlignmentOptions.BottomRight;
					break;
				case AppUI_Control.VerticalAlignment.Midline:
					guiNameLabel.alignment = TextAlignmentOptions.MidlineRight;
					break;
				case AppUI_Control.VerticalAlignment.Capline:
					guiNameLabel.alignment = TextAlignmentOptions.CaplineRight;
					break;
				}
				break;
			case AppUI_Control.HorizontalAlignment.None:
				break;
			}
		}

		public void SetHoverTextTarget(TextMeshProUGUI textTargetForHover)
		{
			if (hoverTextList != null)
			{
				for (int i = 0; i < hoverTextList.Count; i++)
				{
					hoverTextList[i].textTargetForHover = textTargetForHover;
				}
			}
		}

		protected bool SetValue(object newValue)
		{
			if (!IsInitialized)
			{
				return false;
			}
			if (_field != null && !object.Equals(_field.GetValue(_host), newValue))
			{
				_field.SetValue(_host, newValue);
				_host.CallOnDataChanged();
				return true;
			}
			if (_property != null && !object.Equals(_property.GetValue(_host), newValue))
			{
				_property.SetValue(_host, newValue);
				_host.CallOnDataChanged();
				return true;
			}
			return false;
		}

		protected T GetValue<T>()
		{
			return (T)GetValue();
		}

		protected object GetValue()
		{
			if (!IsInitialized)
			{
				return null;
			}
			if (_field != null)
			{
				return _field.GetValue(_host);
			}
			if (_property != null)
			{
				return _property.GetValue(_host);
			}
			return null;
		}

		private void Awake()
		{
			IsInitialized = false;
			OnAwake();
		}

		private void Start()
		{
			OnStart();
		}

		private void Update()
		{
			OnUpdate();
		}

		public static int SortByOrder(AppUIMember m1, AppUIMember m2)
		{
			return m1.order.CompareTo(m2.order);
		}
	}
}
