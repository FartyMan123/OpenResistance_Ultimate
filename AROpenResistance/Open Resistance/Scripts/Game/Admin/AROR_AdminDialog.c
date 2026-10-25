//! Admin dialog (preset "aror_admin" in AROR_SetupDialogs.conf). Same pattern as the setup dialogs:
//! a button records its tag and closes the dialog; the action runs on the admin menu component once it has closed.
class AROR_AdminDialog : SCR_ConfigurableDialogUi
{
	protected AROR_AdminMenuComponent m_AdminMenu;
	protected string m_sPressedTag;

	//------------------------------------------------------------------------------------------------
	void AROR_AdminDialog(AROR_AdminMenuComponent adminMenu)
	{
		m_AdminMenu = adminMenu;
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnButtonPressed(SCR_InputButtonComponent button)
	{
		super.OnButtonPressed(button);

		if (!m_sPressedTag.IsEmpty())
			return;

		m_sPressedTag = GetButtonTag(button);
		Close();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		super.OnMenuClose();

		if (m_AdminMenu && !m_sPressedTag.IsEmpty())
			m_AdminMenu.QueueAction(m_sPressedTag);
	}
}
