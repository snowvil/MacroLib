# MacroLib
Macro Library Class based autohotkey

Use CMacroLib Class 

1. Image Search
  ex)
"`
    CMacroLib m_objMacroLib;
    int nFindPosX = 0;
    int nFIndPosY = 0;
    if(ResultType::OK == m_objMacroLib.ImageSearch(200,1,800,200,_T("*TransFFFFFF Dir\\image.bmp"), nFindPosX, nFIndPosY))
    {
       // find success
    }
    else
    {
      // find failed
    }
"`
2. Control Send
  ex)
    CMacroLib m_objMacroLib;
    m_objMacroLib.ControlSend(m_pGameWnd,{esc}{Shift down}zh{Shift up}{home}{enter},_T(""),_T(""),_T(""),_T(""),false);

3. Set ClipBoard
4. Mouse Move
5. Mouse Click
