/*! \mainpage GLGooey - GUI for OpenGL
 *
 * \section intro Introduction
 *
 * GLGooey is an extensible, small collection of user interface components rendered using OpenGL.
 *
 * \section forum Discussion Board
 *
 * Black Forums kindly host the
 * <a href="http://www.clan-black.com/phpBB2/viewforum.php?f=6">GLGooey discussion board</a>.
 * Even when traffic is low I do drop by regularly, so please feel free to post any comments,
 * questions, criticisms, bug reports, wish lists, whatever.
 * 




 *
 * \section requirements Requirements
 *
 * In order to use GLGooey "out of the box" you will require:
 *
 * - A C++ Compiler including an implementation of the Standard Template Library.
 * - An OS that supports OpenGL 1.2 or higher.
 * - FreeType, which is available from http://www.freetype.org .
 *
 * The following libraries are required in order to facilitate the loading of png images.
 * Note: If you do not require support for image loading, you need only make a small change
 * in the configuration and the following libraries are no longer required (see \ref compileinstructions).
 *
 * - libpng, which is available from http://www.libpng.org/pub/png/ .
 * - zlib, which is available from http://www.gzip.org/zlib/ .
 *
 * It is possible to replace the standard image loading code with the image loader of your choice. Take
 * a look at how the pngImage and devILImage libaries implement the Gooey::Image interface for an example.
 *
 * Furthermore, in order to run any of the examples you will need:
 * - glut, which is available from http://www.sgi.com/software/opengl/glut.html .
 *
 *  Example 4 is an example of a file dialog which uses boost::filesystem. This particular
 * example therefore requires:
 * - boost, which is available from http://www.boost.org .
 *
 * Please note: These last two requirements affect the examples only. The GLGooey
 * library itself is in no way dependent on either boost or glut.
 *
 *
 * \section credits Credits
 *
 * GLGooey was designed and implemented by Niel Waldren. Send any feedback that doesn't warrant
 * posting on the <a href="http://www.clan-black.com/phpBB2/viewforum.php?f=6">GLGooey discussion board</a>
 * to nwaldren@yahoo.com
 *
 * This documentation was created using <a href="http://www.doxygen.org/index.html">Doxygen</a>
 *
 * \subsection libCredits GLGooey contains code from the following libraries:
 *
 *  - sigslot by Sarah Thompson (http://sigslot.sourceforge.net)
 *  - TinyXML by Lee Thomason (http://www.grinninglizard.com)
 *  - FTGL by Henry Maddocks. (http://homepages.paradise.net.nz/henryj/code/)
 *       (\note Up until version 0.10 GLGooey used FTGL for font rendering. FTGL itself proved
 *       too powerful (and thus too heavyweight) for GLGooey's needs, but the OpenGL inclusion
 *       code from FTGL has remained).
 *
 *
 * \subsection thanks Thanks to (in chronological order):
 *  - Michael Gesner for initially getting GLGooey to compile with gcc.
 *  - Xavier Trochu for the step from version 0.04 to 0.05 which was to a great extent his work.
 *  - Bill Baxter for stacks of input and some significant fixes and improvements going
 *     from 0.06 to 0.07, and from 0.07 to 0.08
 *  - Mark Harris for valuable feedback and for contributing the spin box control
 *  - Sebastien Berthet for endless input and assistance.
 *  - the many people who have contributed ideas, reported bugs and generally helped to improve the library
 *
 *
 * \section info Further Info
 *
 * This library is released under the terms of the zlib/libpng license .
 * See the \ref license section for more information.
 *
 * <ul>
 *   <li> \ref license
 *   <li> \ref faq
 *   <li> \ref integration
 *   <li> \ref techinfo
 *   <li> \ref history
 *   <li> \ref compileinstructions
 *   <li> \ref screenshots
 * </ul>
 */




/*! \page faq Frequently Asked Questions
 *
 * Does GLGooey require GLUT to run?
 *
 * No! Not at all. GLUT has just been used in the samples because it's an easy
 * platform independent way of getting OpenGL apps up and running without
 * obfuscating the examples with stacks of code completely unrelated to
 * GLGooey. For further details on the requirements see \ref requirements.
 *
 * <HR>
 *
 * Can I use GLGooey in my project?
 *
 * Use GLGooey however you like. Just take note of the \ref license and the
 * fact that GLGooey uses further libraries whose licenses must of course be respected.
 *
 * <HR>
 *
 * How do I get started if I want to use GLGooey?
 *
 * In the samples directory example 1 is a simple, small and reasonably well documented
 * application demonstrating the basics of using GLGooey. The \ref integration section also
 * contains important information for users getting started with GLGooey.
 *
 * <HR>
 *
 * Will GLGooey compile on my system?
 *
 * GLGooey has been compiled on numerous compilers and operating systems. If
 * for some reason it fails to compile on your system, let me know and if I
 * can do anything about it I will.
 *
 * <HR>
 *
 * Why did you use FreeType instead of the Operating System's font capabilities?
 *
 * GLGooey has been written for maximum independence from any particular compiler
 * or operating system. With that in mind, FreeType is a perfect fit.
 *
 * <HR>
 *
 * Has GLGooey been thoroughly tested?
 *
 * No! But it has been around for a little while now and is used in many different
 * applications.
 *
 * <HR>
 *
 * Older versions allowed me to deactivate RTTI. Is that option no longer available?
 *
 * RTTI is now explicitly required by GLGooey.
 *
 * <HR>
 *
 */





/*! \page integration Using GLGooey in an Application
 *
 * You will need to follow a few basic steps in order to integrate GLGooey with your
 * application (NOTE: GLUT is used as an example here! The procedure is the same
 * whether you actually do everything by hand or use GLUT, SDL, GLFW... or any other
 * OpenGL application framework):
 *
 * <ol>
 * <li> Make sure OpenGL is initialised first!
 * <li> GLGooey must be initialized with a call to \c Gooey::WindowManager::initialize.
 * This function takes four parameters. The first one is the file name of a true
 * type font which will be used as the default font whenever text needs to be drawn
 * and no other font is specified. Furthermore, \c initialize accepts three function
 * pointers as parameters which allow GLGooey to find out whether shift, alt or ctrl keys
 * are pressed. These functions are simple functions that themselves do not take any
 * parameters and return a boolean indicating whether the key in question is currently
 * pressed. Doing this in GLUT might look something like this:
 * \code
 * // First the functions must be defined...
 * bool isShiftPressed()
 * {
 *     return (glutGetModifiers() & GLUT_ACTIVE_SHIFT) != 0;
 * }
 *
 * bool isAltPressed()
 * {
 *     return (glutGetModifiers() & GLUT_ACTIVE_ALT) != 0;
 * }
 *
 * bool isCtrlPressed()
 * {
 *     return (glutGetModifiers() & GLUT_ACTIVE_CTRL) != 0;
 * }
 *
 * // Then in the function where initialization is taking place...
 * ...
 * WindowManager::instance().initialize("myFont.ttf", isShiftPressed, isAltPressed, isCtrlPressed);
 * ...
 * \endcode
 * <li> Messages to be processed by the Gooey::WindowManager must be sent on to the
 * WindowManager class. These messages include the following:
 *  <ul>
 *    <li> mouse movement and clicks
 *    <li> keyboard messages
 *    <li> resizing of the window in which GLGooey is running.
 *  </ul>
 *
 * Doing this in a GLUT-based application (\e Note: This is just an example. GLGooey does
 * not require GLUT in any way. Only the samples actually require GLUT) would look something like this:
 * \code
 * void myGlutMouse(int button, int state, int x, int y)
 * {
 *     switch(button)
 *     {
 *     case GLUT_LEFT_BUTTON:
 *         if(state == GLUT_UP)
 *             WindowManager::instance().onLeftButtonUp(x, y);
 *         else
 *             WindowManager::instance().onLeftButtonDown(x, y);
 *        break;
 *     case GLUT_MIDDLE_BUTTON:
 *        if(state == GLUT_UP)
 *             WindowManager::instance().onMiddleButtonUp(x, y);
 *         else
 *             WindowManager::instance().onMiddleButtonDown(x, y);
 *         break;
 *     case GLUT_RIGHT_BUTTON:
 *        if(state == GLUT_UP)
 *             WindowManager::instance().onRightButtonUp(x, y);
 *         else
 *             WindowManager::instance().onRightButtonDown(x, y);
 *         break;

 *     }
 * }
 *
 * void myGlutMotion(int x, int y)
 * {
 *     WindowManager::instance().onMouseMove(x, y);
 * }
 *
 *
 * void myGlutKey(unsigned char key, int x, int y)
 * {
 *     WindowManager::instance().onChar(key);
 * }
 *
 * void myGlutSpecial(int key, int x, int y)
 * {
 *     WindowManager::instance().onKeyDown(key);
 * }
 *
 * void myGlutReshape( int x, int y )
 * {
 *    WindowManager::instance().applicationResized(x, y);
 * }
 * \endcode
 *
 * \note GLGooey uses the same key codes as GLUT, if your app's message
 *       function is giving you the keys with different key codes, you'll
 *       have to "translate" them to the values defined in KeyMessage.h
 *
 * \note It is of particular importance to make sure you call \c applicationResized
 * whenever the size of the window in which GLGooey is running changes. It should
 * normally be put in the \e onResize method of that window. Not informing GLGooey of
 * the size of the application window is one of the most common causes of people
 * getting started with GLGooey not being able to see anything!
 *
 * <li>Call update() from the WindowManager instance whenever you want
 * the UI to be processed and rendered.
 * \code
 * WindowManager::instance().update();
 * \endcode
 *
 * <li>When you're finished with GLGooey you must make a call to \c terminate.
 * \code
 * WindowManager::instance().terminate();
 * \endcode
 *
 * </ol>
 *
 *
 */

/*! \page screenshots Screenshots from the Examples
 *
 * <img src="./../../images/example1.jpg" alt="shot2" align="middle" border=0 width=442 height=445>
 * <p>
 * A simple console using Gooey controls to allow the user to edit strings and append
 * them to a list box. This screenshot was taken from example1 in the samples directory.
 *
 * <hr>
 *
 * <img src="./../../images/example2.jpg" alt="shot3" align="middle" border=0 width=372 height=377>
 * <p>
 * A similar example to the above, but using a different layouter, and skin.
 * This screenshot was taken from example2 in the samples directory.
 *
 * <hr>
 *
 * <img src="./../../images/example3.jpg" alt="shot1" align="middle" border=0 width=755 height=405>
 * <p>
 * A 3D wireframe sphere with a selection of Gooey windows showing semi-transparent
 * in the foreground. This app also demonstrates the use of the Gooey::ComplexGridLayouter
 * class for placing windows in a Gooey::Panel. This screenshot was taken from example3
 * in the samples directory.
 *
 * <hr>
 *
 * <img src="./../../images/example3b.jpg" alt="shot1" align="middle" border=0 width=689 height=418>
 * <p>
 * In GLGooey you can choose whether to let users move windows or not. Here the
 * windows from the above example have been shifted around a little.
 *
 * <hr>
 *
 * <img src="./../../images/example3c.jpg" alt="shot1" align="middle" border=0 width=753 height=408>
 * <p>
 * Example3 with a different skin.
 *
 * <hr>
 *
 * <img src="./../../images/example4.jpg" alt="shot1" align="middle" border=0 width=640 height=480>
 * <p>
 * A shot from example4 which shows the GLGooey file dialog.
 *
 */

/*! \page techinfo Some Technical Information
 *
 * \section gooeyclipboard Copy and Paste in GLGooey
 *
 * As of version 0.08, GLGooey's edit field supports copy and paste. The clipboard is,
 * however, a rather OS-dependent beast, and this poses a problem for GLGooey which is
 * trying to be as OS-independent as possible. So how is clipboard functionality realized
 * in GLGooey?
 *
 * Basically, the clipboard is internal. The Gooey::WindowManager object maintains the
 * text that is currently in the clipboard, so any copying and pasting can only be done
 * within GLGooey. This means that you cannot copy some text from a widget in your OS
 * and paste it into a GLGooey edit field, at least not without a bit of extra work.
 * In order to facilitate the connection to a particular OS for those who might need
 * it, the window manager emits a signal whenever the contents of GLGooey's internal
 * clipboard change. So if you want to allow copying and pasting between GLGooey and
 * the host OS, you have to react to that signal to inform the OS of a copy action
 * within GLGooey, and whenever the contents of the OS's clipboard change, you'll
 * have to use the window manager's copyToClipboard function to inform GLGooey. Please
 * remember that GLGooey's edit field is a simple single line edit field!
 *
 *
 * \section gooeysigslot Signals and Slots in GLGooey
 *
 * There's plenty of info on Signals and Slots on the web, so I won't bother explaining
 * the concept here. If you are not familiar with the concept I can recommend the QT documentation
 * (http://www.trolltech.com/) and the documentation Sarah Thomson wrote for the sigslot library
 * (http://sigslot.sourceforge.net/). Because GLGooey's signals and slots mechanism \e is basically
 * sigslot, I'd recommend reading the sigslot documentation for a more in depth discussion of what
 * is described in the following section.
 *
 * \subsection signalresponse Responding to a Signal
 *
 * Most GLGooey windows emit a signal when certain events take place. Let's consider the simple
 * example of a button:
 *
 * On consulting the documentation for a button you will notice that the there is a signal
 * declared as follows \code Signal0 pressed; \endcode This signal is (somewhat logically)
 * emitted whenever the user presses the button. In order to respond to this signal you must
 * write a method which adheres to the following rules:
 * - the \c 0 suffix of \c Signal0 indicates that the responding slot must have 0 parameters
 * - the responding slot may return either \c void or a reference to the class in which it is
 *     declared.
 * - the class which is to hold the slot (i.e. the class in which the slot is declared) must
 *     derive from \c SlotHolder. (Normally this is not an issue because most slots will be
 *     contained in classes derived from \c Window which is itself in turn derived from \c
 *     SlotHolder).
 *
 * So, once you have such a slot, how do you connect it to a signal. An (adapted) exerpt
 * from Example2 demonstrates this:
\code
class Console : public FrameWindow
{
public:
    // The constructor creates windows and "wires up" the signals and slots
    Console() : FrameWindow(Gooey::Rectangle(50, 50, 400, 400), 0, "Console")
    {
        ...
        _listBox     = new ListBox(_panel, Gooey::Rectangle(0, 0, 600, 240));
        _editField   = new EditField(_panel, "Edit Field:");
        _doneButton  = new Button(_panel, "Done");
        _clearButton = new Button(_panel, "Clear");

        // wire up the signal/slot connection
        _doneButton->pressed.connect(this, &Console::doneButtonPressed);
        _editField->returnPressed.connect(this, &Console::doneButtonPressed);
        _clearButton->pressed.connect(_listBox, &ListBox::removeAllStrings);
        ...
    }
    ...
    // This method is a typical slot
    void doneButtonPressed()
    {
        // append the contents of the edit field to the list box and clear the edit field
        ...
    }
    ...
};
\endcode
 * The first four lines in the constructor create the windows involved, a list box, an edit field and
 * a "done" button - which on being pressed should append the contents of the edit field to the list box
 * and then clear the edit field. The last window is a "clear" button which should clear
 * the contents of the list box.
 *
 * The next lines actually connect the signals and slots. For instance
 * \code _doneButton->pressed.connect(this, &Console::doneButtonPressed); \endcode
 * connects the \c doneButtonPressed slot declared directly below the constructor in
 * the above example to the \c pressed signal emitted by the "done" button. Now, whenever the
 * user presses the "done" button, the \c doneButtonPressed method will be called. Similarly,
 * the \c returnPressed signal is also connected to the \c doneButtonPressed method. Therefore,
 * pressing return from the edit field will have the same effect as pressing the done button.
 *
 * The line
 * \code _clearButton->pressed.connect(_listBox, &ListBox::removeAllStrings); \endcode
 * is the only line required to implement the functionality of the "clear" button. This is because
 * member functions of GLGooey windows are in many cases themselves slots (i.e. they return void or
 * a reference to the class in which they reside - usually the latter). So we can simply wire
 * the \c pressed signal of the "clear" button directly to the \c removeAllStrings method in
 * \c ListBox. Now whenever a user presses the "clear" button, \c _listBox->removeAllStrings() is
 * called.
 *
 *
 * \subsection diffsigslot Differences Between Sigslot and Signals and Slots in GLGooey
 *
 * As mentioned above, the signals and slots mechanism is basically the sigslot library
 * written by Sarah Thomson. Apart from changing the coding style to suit the rest of
 * GLGooey, I made the following changes to sigslot:
 *  - Removed all multithreading code. GLGooey is NOT, nor does it intend to be thread safe. A
 *     lot of the code in sigslot is dedicated to making the mechanism thread safe - something
 *     I just don't need. I chose to remove the thread safety code in order to facilitate the
 *     readability and as a result the usablility of the library within GLGooey.
 *  - sigslot requires all slots to return \c void. However GLGooey very often returns a
 *     reference to the host class from member functions. Therefore I decided to add support
 *     for slots which return such references.
 *
 *
 */


/*! \page history GLGooey History
 *
 * \section v010 Version 0.10
 *  <ul>
 *    <li> Added generic serialization which allows skinning of individual windows
 *    <li> The file dialog has been removed from the actual library and is now an example of using the library (see example4)
 *    <li> The file dialog now uses boost::filesystem for platform independent file handling
 *    <li> FTGL has been replaced by a small streamlined font rendering system built specifically for its intended purpose
 *    <li> Core functionality has been factored out of the GLGooey library and placed in a more low level library of its own
 *    <li> Image loading and font rendering are now provided via so-called \e services which can be replaced independently of the library
 *    <li> Positions are now internally floats and the rounding issues that cause anomalies in the rendering of earlier versions have been resolved
 *    <li> Added BoxLayouter
 *    <li> The code layout has been restructured making it easier to maintain
 *    <li> The coding style has been made consistent throughout the library
 *    <li> Reverted back to using makefiles
 *    <li> included MSVC 6 project files in distribution
 *    <li> Changed the license to the more liberal and less complicated zlib/libpng license
 *  </ul>
 *
 * \section v009 Version 0.09
 *  <ul>
 *    <li> Added initial skinning capablities
 *    <li> Fixed a bug in the file dialog which caused a crash under windows if the user tried to go "up" from the root directory
 *    <li> Combo Box now closes the drop down window if the selected item is reselected. Previously the drop down closed only if the selection changed
 *    <li> Introduced enabling and disabling of windows
 *    <li> Started using Paul Nettle's memory manager (HTTP://www.FluidStudios.com) to monitor memory issues
 *    <li> Fixed a nasty bug in the EditField class which was causing release builds to crash as a result of unitialized variables.
 *    <li> Cleaned some superfluous includes from the examples
 *    <li> Started using jam instead of make
 *    <li> Doxygen - already an excellent tool - has become even better since I started on GLGooey, so I've changed the comments in many places
 *             to take advantage of recent features
 *  </ul>
 *
 * \section v008 Version 0.08
 *  <ul>
 *    <li> Mark Harris contributed a new SpinBox control
 *    <li> Windows now have a \c recommendedSize() method which specifies the default size of the window 
 *           until the user sets a size. It is therefore for example no longer necessary to set the size 
 *           of each label in a dialog explicitly.
 *    <li> Added selection to the edit field using either the mouse or the keyboard
 *    <li> Added copy and paste. See \ref gooeyclipboard
 *    <li> Edit field now supports CTRL-C, CTRL-V, CTRL-X, CTRL-A and CTRL-CURSOR_KEY inputs
 *    <li> WindowManager's event handling methods now return a boolean value indicating whether the message was handled
 *    <li> New Slider control
 *    <li> Added listOpened and listClosed signals to the combo box
 *    <li> Added fileSelected and directoryChanged signals to the file dialog
 *    <li> Added leftClicked, rightClicked and selectionChanged signals to the list control
 *    <li> Added selectionChanged signal to the multi-text button
 *    <li> Added sliderPressed, sliderReleased and sliderMoved signals to Slideables (i.e. ScrollBar and Slider)
 *    <li> FocusListener and EditListener: dispatchMessage() needed an extra const to compile
 *           (also a hold-over from v0.06) - thanks to Mark Harris for spotting the problem in
 *           FocusListener. I hope I've got them all now!!
 *  </ul>
 *  The following changes were made by Bill Baxter
 *  <ul>
 *    <li> Fixed a bug in StaticText which was causing an infinite loop if the width of the windows was less than 1 pixel
 *    <li> Save and restore the TexEnv state around WindowManager updates
 *    <li> Added response to double-click
 *    <li> Better filter parsing in FileDialog. Now you can make a filter like "Images (*.png, *.jpg, *.bmp)"
 *           and it will actually filter for those 3 types of files at once.
 *    <li> Better directory handling and parsing in FileDialog. Now you can give the file
 *           dialog a relative path, like "../../images" and it translates that into
 *           the correct absolute path.
 *    <li> In File Dialog: Changed buttons to connect to the "clicked()" signal.
 *    <li> Changed some of the simple const methods in FileDialog to static class methods since
 *           they don't depend on any instance data.
 *    <li> SlideableMessage.h: dispatchMessage() needed an extra const to compile (hold-over from v0.06).
 *    <li> GLGooey.h: added new class headers like FileDialog.h
 *    <li> BasicButton.{h,cpp}: added a clicked signal
 *    <li> Checkbox.{h,cpp}: added a toggled signal and use button clicked signal rather than released signal
 *    <li> Slideable.{h,cpp}: added a valueChanged signal
 *  </ul>
 *
 * \section v007 Version 0.07
 *  <ul>
 *    <li> Caret placement per mouse click and handling of "Home" and "End" keys added to EditField
 *    <li> Fonts now calculate and cache the vertical space they require on a line. Previously
 *           windows were calculating this repeatedly for themselves.
 *    <li> Text wraps automatically in StaticText windows
 *    <li> Introduced a signals and slots mechanism
 *    <li> New ComboBox control
 *    <li> New file selection dialog
 *    <li> New example demonstrating the file selection dialog
 *    <li> Implemented movable frame windows
 *    <li> Many people pointed out a crash on exit. (I couldn't reproduce the crash, so thanks to
 *           Gabor Dorka for providing the info I needed to fix the bug!). This was a classic example
 *           of destruction order of singletons causing problems. The problem has been solved by
 *           making objects that were singletons into members of WindowManager.
 *    <li> Fixed a memory leak in Panel (also pointed out by Gabor Dorka)
 *    <li> Button (a standard push button) had grown to become the base class of CheckBox and
 *           MultiTextButton. This is a design no-no, so I've created an abstract base class BasicButton
 *           from which Button, CheckBox and MultiTextButton now derive.
 *    <li> Fixed a capture issue in the button class which was causing the scroll bar to behave strangely
 *    <li> Fixed a bug in the ComplexGridLayouter which was preventing the same layouter object from
 *           working correctly multiple times.
 *    <li> Got rid of the compilation history page from the documentation
 *  </ul>
 *  The following changes were made by Bill Baxter
 *  <ul>
 *    <li> Created a convenience header \c glgooey.h for including the entire library in one swoop
 *    <li> Fixed an issue in the title bar of frame windows where large fonts were being unnecessarily clipped
 *    <li> Added a \c focusedEditFieldColor to the property scheme
 *    <li> Wrapped font instantiation in PropertyScheme.cpp in try/catch so you don't HAVE to have the default font available if you don't want it
 *    <li> Changed the single font dir to a full font path in \c Font::initializeSystem
 *    <li> added a check for the trailing '/' in the font path. It used to fail if the final dir separator wasn't there
 *  </ul>
 * \section v006 Version 0.06
 *  <ul>
 *    <li> Fixed an issue with the WindowManager attempting to access deleted Windows (Thanks to Jurgen van Gael informing me of the problem)
 *    <li> Removed a nasty memory leak from the drawing code (Thanks to Richard van Eijbergen for spotting the leak)
 *    <li> changed the specification of font names in the Font class to require the full filename. ".ttf" is no longer automatically appended
 *    <li> cleaned up some drawing code particularly in FrameWindow
 *    <li> turned all #defines into constants
 *  </ul>
 * \section v005 Version 0.05
 *  The main focus of this version has been to ensure that the same version will compile on the most commonly used platforms
 *  <ul>

 *    <li> Removed all references to features not supported by the common ground between the StlPort, gcc 2.95, gcc 3.0, MSVC 6 and MSVC .NET standard library implementations
 *    <li> Moved all initializations of static constant integer values out of the class definitions and into the implementations
 *    <li> Introduced a compilation history page to the documentation
 *    <li> Removed all windows specific code from the samples
 *    <li> Included some true-type fonts in the distribution for use by the samples
 *  </ul>
 *  The following changes were made (or in a few cases suggested) by Xavier Trochu
 *  <ul>
 *    <li> Changed the position of checkForWindowBelowCursor() in onMouseMove(), and added this call in all mouse related WindowManager callbacks
 *    <li> Changed friend declarations from <tt> friend SomeClass </tt> to <tt> friend class SomeClass </tt>
 *    <li> Replaced the Right Mouse Button Handling of the MultiTextButton by a copy of the Left Mouse Button Handling of Button

 *    <li> Made CheckBox inherit from Button and the CheckBox listener inherit from ActionListener
 *    <li> WindowManager has 3 new public methods for dealing with mouse capture
 *    <li> Made references to messages passed to message handling functions constant
 *    <li> Reordering of initializers in constructors to match the corresponding declarations
 *    <li> Changed the contents of OpenGL.h to share the OpenGL inclusion code with FTGL
 *    <li> Moved class specific window listeners out of the headers and into the implementation files
 *    <li> Derived Gooey::StandardException from std::exception and replaced the \c message() method with \c what()
 *    <li> Changed brute force downcasts into dynamic casts in message listeners
 *    <li> Some bugfixes and code improvements
 *  </ul>
 *
 * \section v004 Version 0.04

 *  <ul>
 *    <li> Introduced StaticBitmap windows for displaying bitmaps
 *    <li> Introduced a CheckBoxGroup class to provide a simple means of implementing radio buttons

 *    <li> Introduced FTGL as a layer between GLGooey and FreeType for drawing text
 *    <li> Introduced a consistent message handling system
 *    <li> Introduced Layouter classes for the layout of controls in panels
 *    <li> Encapsulated property schemes in their own class
 *    <li> Basic clean up and restructure
 *  </ul>
 *
 * \section v002 Version 0.02
 *  <ul>
 *    <li> fixed the "scroll too far" bug in the list control and list box classes
 *    <li> removed a redundant dependancy in the rectangle class
 *    <li> removed some signed / unsigned comparison issues
 *    <li> made friend functions explicit template functions in point.h
 *    <li> no longer use static colors to initialize static default colors in Window.cpp
 *    <li> introduced showing and hiding of windows
 *    <li> implemented a progress bar which supports both vertical and horizontal orientations
 *    <li> included a formal license. (Thank you to Marcelo Magallon for pointing out the absence of a formal license)
 *  </ul>
 *
 * \section v001 Version 0.01

 * Original release
 *
 */

/*!  \page compileinstructions Compilation Instructions
 *
 * GLGooey uses several external libraries, so getting it to run can be quite trying. It is
 * also difficult to explain how to get things to run because there are endless different variations
 * depending on what platform is being used, which libraries are installed and where they are
 * installed. If you are not familiar with installing libraries and making them accessible to
 * applications you will almost certainly have difficulties getting GLGooey to compile. Try the
 * <a href="http://www.clan-black.com/phpBB2/viewforum.php?f=6">GLGooey discussion board</a> if
 * you stumble upon problems.
 *
 * The following instructions for the two most commonly used compilers assume that you are familiar
 * with the installation and use of external libraries:
 *
 * <ol>
 *   <li>\b gcc: make and install the required libraries (see \ref requirements), then in
 *      the main glgooey directory
 *      \verbatim
           >./configure
           >./make \endverbatim
 *      This assumes that the include and lib directories of required libraries are on your path.
 *      Example 4 assumes boost is installed in <tt>/usr/local/include/boost-1_31</tt>. You will have to tweak the
 *      makefile if you use a different directory.
 *   <li>\b msvc: This is the way I get GLGooey running on msvc - it is not the only way:
 *      <ul>
 *        <li>Define a directory where dlls will land (I use <tt>c:/Programs/GnuWin32/bin</tt>) and add that
 *            directory to your path.
 *        <li>Define a directory where lib files will land (I use <tt>c:/Programs/GnuWin32/lib</tt>) and add
 *            it to the IDE's list of directories to search for lib files
 *        <li>Define a directory where include files will land (I use <tt>c:/Programs/GnuWin32/include</tt>) and add
 *            it to the IDE's list of directories to search for include files
 *        <li>Download and install FreeType, libpng and zlib such that libs, dlls and headers land in the
 *            respective directories defined above
 *        <li>FreeType installs further headers in a subdirectory called <tt>freetype2</tt>. This directory must
 *            also be added to the IDE's list of directories to search for include files.
 *        <li>Download and install GLUT so that headers, libs and dlls are in the same directories as their OpenGL
 *            equivalents.
 *            \note GLGooey assumes that glut.h is in a directory called "GL". I.e. if glut is in
 *            <tt>c:/some_path/GL/</tt>, <tt>c:/some_path/</tt> must be in the IDE's list of directories to search for include files)
 *        <li>Download and install boost. Add the boost include path to your IDE's list of directories to search
 *            for include files
 *        <li>Open <tt>glgooey/msvc/glgooey/glgooey.dsw</tt> workspace in the IDE. If all has gone well it should now
 *            compile and run. \note You may find you'll have to set the working directory of the example applications
 *            to suit the paths which are hard coded in the examples for simplicity. The paths start with ../../samples
 *            in order to reach the samples directory. So if you set the working directory to go back twice in order
 *            to reach the main glgooey directory, things should be fine!
 *      </ul>
 * </ol>
 *
 *   If you do not want to support images, you needn't bother with zlib and libpng, but you must change the
 * GLGooey configuration such that the examples include the nilImage library instead of the pngImage library.
 * Alternatively, you can use the provided <a href="http://openil.sourceforge.net/">DevIL</a> service or, for
 * instance, if you already have image loading code in your application, you can take a look at the image loading
 * services provided and implement a service using your existing image loading code - it's pretty straightforward!
 *
 *   There can only ever be one image service linked to the client application at any one time. The application
 * should, strictly speaking, never need to know which image service is actually doing the image loading.
 *
 */




/*! \page license License and Disclaimer
 *
 *  \verbatim
 This software is provided 'as-is', without any express or implied warranty. In no event will
 the authors be held liable for any damages arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose, including commercial
 applications, and to alter it and redistribute it freely, subject to the following restrictions:

     1. The origin of this software must not be misrepresented; you must not claim that you
        wrote the original software. If you use this software in a product, an acknowledgment
        in the product documentation would be appreciated but is not required.

     2. Altered source versions must be plainly marked as such, and must not be misrepresented
        as being the original software.

     3. This notice may not be removed or altered from any source distribution.
   \endverbatim
 *

 */

