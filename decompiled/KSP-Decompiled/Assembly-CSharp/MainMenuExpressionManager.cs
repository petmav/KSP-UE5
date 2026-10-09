using UnityEngine;

public class MainMenuExpressionManager : MonoBehaviour
{
	[SerializeField]
	private Animator animationControl;

	[SerializeField]
	private float expressionValue;

	[SerializeField]
	private float expressionVariance = 0.5f;

	[SerializeField]
	private float expressionSecondVariance = 0.3f;

	private void Start()
	{
		animationControl.SetFloat("Expression", expressionValue);
		animationControl.SetFloat("Variance", expressionVariance);
		animationControl.SetFloat("SecondaryVariance", expressionSecondVariance);
	}
}
