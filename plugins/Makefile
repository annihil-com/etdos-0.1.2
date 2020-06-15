all:
	gcc -shared -fPIC -fvisibility=hidden -lm -I../../etdos null/null.c -o nullplugin.so
	gcc -shared -fPIC -fvisibility=hidden -lm -I../../etdos follow/follow.c -o followplugin.so
	gcc -shared -fPIC -fvisibility=hidden -lm -I../../etdos aimbot/aimbot.c -o aimbotplugin.so
	gcc -shared -fPIC -fvisibility=hidden -lm -I../../etdos jaymod2x/jaymod2x.c -o jaymod2xplugin.so
	gcc -shared -fPIC -fvisibility=hidden -lm -I../../etdos spam/spam.c -o spamplugin.so
	gcc -shared -fPIC -fvisibility=hidden -lm -I../../etdos medicbot/medicbot.c -o medicbotplugin.so
clean:
	rm *.so
