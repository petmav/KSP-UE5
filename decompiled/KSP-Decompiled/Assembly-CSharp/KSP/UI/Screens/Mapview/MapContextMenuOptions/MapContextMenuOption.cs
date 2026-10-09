using System;

namespace KSP.UI.Screens.Mapview.MapContextMenuOptions
{
	public abstract class MapContextMenuOption : DialogGUIButton
	{
		public MapContextMenuOption(string caption)
			: base(caption, delegate
			{
			}, dismissOnSelect: true)
		{
			onOptionSelected = OnSelect;
			OptionInteractableCondition = (Func<bool>)Delegate.Combine(OptionInteractableCondition, new Func<bool>(CheckEnabled));
			OptionEnabledCondition = (Func<bool>)Delegate.Combine(OptionEnabledCondition, new Func<bool>(CheckAvailable));
		}

		protected abstract void OnSelect();

		private bool CheckEnabled()
		{
			string fbText = string.Copy(OptionText);
			bool result = OnCheckEnabled(out fbText);
			if (fbText != OptionText)
			{
				SetOptionText(fbText);
			}
			return result;
		}

		protected virtual bool OnCheckEnabled(out string fbText)
		{
			fbText = OptionText;
			return true;
		}

		public virtual bool CheckAvailable()
		{
			return true;
		}
	}
}
