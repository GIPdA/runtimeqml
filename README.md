# Runtime QML for Qt

**Written by**: *Benjamin Balga.*
**Copyright**: ***2016***, *Benjamin Balga*, released under BSD license.


## About

This is a base project to get runtime QML reload in your Qt project.
It allows you to reload all QML code at runtime, without recompiling or restarting your app, saving time.

With auto-reload, QML files are watched (based on the QRC file) and reloaded when you save them, or can trigger it manually.

On reload, all windows are closed, and the main window is reloaded. All "QML-only data" is lost, so use links to C++ models/properties as needed.

It only works with Window component as top object, or QQuickWindow subclasses.


## How to use it in your project

Copy the ```runtimeqml``` folder into your project and import the ```.pri``` project file into your ```.pro``` file:

	include(runtimeqml/runtimeqml.pri)


## Usage

Include ```runtimeqml.h``` header file, and create the RuntimeQML object (after the QML engine) :

	RuntimeQML *rt = new RuntimeQML(&engine, TOSTRING(SOURCE_PATH) "/qml.qrc");

The second argument is the path to your qrc file listing all your QML files, needed for the auto-reload feature only. You can omit it if you don't want auto-reload.
```SOURCE_PATH``` is defined in the .pro file to the .pro path, just to not have to manually set an absolute path...


Set the "options" you want, or not:

	rt->noDebug(); // Removes debug prints
	rt->setAutoReload(true); // Enable auto-reload (begin to watch files)
	//rt->setCloseAllOnReload(false); // Don't close all windows on reload. Not advised!
	rt->setMainQmlFilename("main.qml"); // This is the file that loaded on reload, default is "main.qml"

For the auto-reload feature:

	rt->addSuffix("conf"); // Adds a file suffix to the "white list" for watched files. "qml" is already in.
    rt->ignorePrefix("/test"); // Ignore a prefix in the QRC file.
    rt->ignoreFile("Page2.qml"); // Ignore a file name (you can add a path to filter more)


Then load the main QML file :

	rt->reload();
    
And you're all set!


## Manual reload

Add the RuntimeQML object to the QML context:

	engine.rootContext()->setContextProperty("RuntimeQML", rt);
	
Trigger the reload when and where you want, like with a button:

	Button {
        text: "Reload"
        onClicked: {
            RuntimeQML.reload();
        }
    }

You can do it in C++ too, of course.



## License
See LICENSE file.
