using System.Collections.Generic;

public class TutorialPageConfig
{
	public List<TutorialPageConfig> tutoStepsConfig;

	public string pageId { get; private set; }

	public string pageTitleLocId { get; private set; }

	public string pageDialogLocId { get; private set; }

	public KFSMStateChange onEnterCallback { get; private set; }

	public METutorialScenario.TutorialButtonType pageButtonType { get; set; }

	public TutorialPageConfig(string pageId, string pageTitleLocId, string pageDialogLocId, KFSMStateChange onEnterCallback, METutorialScenario.TutorialButtonType pageButtonType = METutorialScenario.TutorialButtonType.Next)
	{
		this.pageId = pageId;
		this.pageTitleLocId = pageTitleLocId;
		this.pageDialogLocId = pageDialogLocId;
		this.onEnterCallback = onEnterCallback;
		this.pageButtonType = pageButtonType;
	}

	protected TutorialPageConfig()
	{
		InitializeTutoStepsConfig();
		AddTutorialStepConfig();
	}

	protected virtual void AddTutorialStepConfig()
	{
	}

	private void InitializeTutoStepsConfig()
	{
		tutoStepsConfig = new List<TutorialPageConfig>();
	}

	protected void AddTutorialStepConfig(string pageId, string pageTitleLocId, string pageDialogLocId, KFSMStateChange onEnterCallback, METutorialScenario.TutorialButtonType pageButtonType = METutorialScenario.TutorialButtonType.Next)
	{
		TutorialPageConfig item = new TutorialPageConfig(pageId, pageTitleLocId, pageDialogLocId, onEnterCallback, pageButtonType);
		tutoStepsConfig.Add(item);
	}

	protected void OnEnterEmpty(KFSMState state)
	{
	}
}
