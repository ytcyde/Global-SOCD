#HotIf GetKeyState("d", "P")
~a::Send "{d up}"
~a up::Send "{d down}"
#HotIf
#HotIf GetKeyState("a", "P")
~d::Send "{a up}"
~d up::Send "{a down}"
#HotIf