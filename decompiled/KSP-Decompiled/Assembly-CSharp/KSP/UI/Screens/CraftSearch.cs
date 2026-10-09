using System;
using System.Collections;
using KSP.Localization;
using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	public class CraftSearch : MonoBehaviour
	{
		public delegate void HasFiltered(bool filtered);

		public static CraftSearch Instance;

		[Tooltip("The search input field")]
		[Header("Inspector assigned")]
		public TMP_InputField searchField;

		[SerializeField]
		[Tooltip("The display area for the vehicles")]
		private GameObject CraftlistArea;

		[Tooltip("(seconds) Delay in checks for keystrokes")]
		[SerializeField]
		private float searchKeystrokeDelay = 0.25f;

		public HasFiltered hasFiltered;

		[NonSerialized]
		public const string SEARCH_FIELD_CONTROLLOCK_ID = "CraftSearchFieldTextInput";

		protected Image searchFieldBackground;

		protected PointerClickHandler searchFieldClickHandler;

		protected float searchTimer;

		protected Coroutine searchRoutine;

		private string previousSearch;

		private static CraftBrowserDialog craftBrowserDialog;

		private static GameObject craftBrowserContent;

		private bool IsDifferentSearch => previousSearch != searchField.text;

		private void Awake()
		{
			Instance = this;
			if (craftBrowserDialog == null)
			{
				craftBrowserDialog = Instance.GetComponent<CraftBrowserDialog>();
				craftBrowserContent = CraftlistArea.gameObject;
			}
			searchField = GetComponentInChildren<TMP_InputField>();
			searchFieldBackground = searchField.GetComponent<Image>();
			searchField.onValueChanged.AddListener(SearchField_OnValueChange);
			searchField.onEndEdit.AddListener(SearchField_OnEndEdit);
			searchFieldBackground = searchField.GetComponent<Image>();
			searchFieldClickHandler = searchField.gameObject.AddComponent<PointerClickHandler>();
			searchFieldClickHandler.onPointerClick.AddListener(SearchField_OnClick);
		}

		private IEnumerator Start()
		{
			yield return null;
			searchField.gameObject.SetActive(value: false);
			searchField.gameObject.SetActive(value: true);
		}

		private void OnDestroy()
		{
			StopSearch();
			if (Instance != null && Instance == this)
			{
				Instance = null;
			}
		}

		public void OnDisable()
		{
			searchField.text = string.Empty;
			StopSearch();
		}

		public void StopSearch(bool isPaused = false)
		{
			if (searchRoutine != null)
			{
				StopCoroutine(searchRoutine);
				searchRoutine = null;
			}
			if (!isPaused)
			{
				searchFieldBackground.color = Color.white;
				previousSearch = string.Empty;
			}
		}

		protected void SearchField_OnEndEdit(string s)
		{
			InputLockManager.RemoveControlLock("CraftSearchFieldTextInput");
		}

		protected void SearchField_OnValueChange(string s)
		{
			SearchStart();
		}

		protected void SearchField_OnClick(PointerEventData eventData)
		{
			if (eventData.button == PointerEventData.InputButton.Left)
			{
				InputLockManager.SetControlLock(ControlTypes.KEYBOARDINPUT, "CraftSearchFieldTextInput");
				SearchStart();
			}
		}

		public virtual void SearchStart()
		{
			if (searchRoutine != null)
			{
				StopSearch();
			}
			searchTimer = Time.realtimeSinceStartup;
			if (searchRoutine == null && base.gameObject.activeInHierarchy)
			{
				searchFieldBackground.color = Color.green;
				searchRoutine = StartCoroutine(SearchRoutine());
			}
		}

		protected IEnumerator SearchRoutine()
		{
			while (searchTimer + searchKeystrokeDelay > Time.realtimeSinceStartup)
			{
				yield return null;
			}
			bool filtered = false;
			string searchTerm = searchField.text;
			for (int i = craftBrowserContent.transform.childCount; i > 0; i--)
			{
				int index = i - 1;
				Transform child = craftBrowserContent.transform.GetChild(index);
				CraftEntry component = child.GetComponent<CraftEntry>();
				if (!(component == null))
				{
					bool flag = CraftMatchesSearch(component, searchTerm);
					child.gameObject.SetActive(flag);
					if (!flag && !filtered)
					{
						filtered = true;
					}
					yield return null;
				}
			}
			if (hasFiltered != null)
			{
				hasFiltered(filtered);
			}
			if (string.IsNullOrWhiteSpace(searchTerm) && IsDifferentSearch)
			{
				StopSearch();
			}
			previousSearch = searchTerm;
		}

		protected bool CraftMatchesSearch(CraftEntry craft, string searchTerm)
		{
			if (string.IsNullOrWhiteSpace(searchTerm))
			{
				return true;
			}
			string text = Localizer.Format(craft.craftProfileInfo?.description);
			if (craft.name.IndexOf(searchTerm, StringComparison.OrdinalIgnoreCase) <= -1)
			{
				return text.IndexOf(searchTerm, StringComparison.OrdinalIgnoreCase) > -1;
			}
			return true;
		}
	}
}
