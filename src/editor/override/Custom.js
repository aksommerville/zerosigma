/* Custom.js
 * This exists only to be overridden by games' editors.
 */
 
import { Data } from "../js/Data.js";
import { Dom } from "../js/Dom.js";
import { MapRes } from "../js/map/MapRes.js";
 
export class Custom {
  static getDependencies() {
    return [Data, Dom];
  }
  constructor(data, dom) {
    this.data = data;
    this.dom = dom;
  }
  
  /* Return an array of {
   *   op: string // unique
   *   label: string // for display
   *   fn: () => void
   * }
   * Return in the order you want them to appear. Custom actions come before standard ones in the menu.
   */
  getActions() {
    return [
      { op: "flowersByMap", label: "Flowers by map", fn: () => this.flowersByMap() },
    ];
  }
  
  /* Array of class implementing:
   *   static checkResource({path,type,rid,serial}): (false,1,2) = (no,yes,prefer)
   *   setup({path,type,rid,serial})
   *
   * These should inject Data and inform it when dirty.
   * See egg/src/editor/js/TextEditor.js for a nice simple example (and ignore its checkResource; yours should be much simpler).
   *
   * Order matters! When searching for a default editor, the first to return >=2 wins, then the first to return 1.
   * Standard editors will only win if all of your editors return false at checkResource().
   *
   * In most cases, checkResource() should just return 2 if type matches and 0 otherwise.
   * Doing a more involved format check is of course possible but kind of pointless, that's what type is for.
   */
  getEditors() {
    return [];
  }
  
  /* To get custom point commands appearing in MapEditor instead of the "?" tile,
   * pick off your interesting annotations (usually by annotiation.opcode), and return a Canvas or Image.
   * You can't override things the editor knows about: sprite, door, door:exit
   */
  renderMapEditorAnnotation(annotation, command, map) {
    return null;
  }
  
  /* Begin custom code.
   *************************************************************************************************/
   
  flowersByMap() {
    let report = "";
    let total = 0;
    for (const res of this.data.resv) {
      if (res.type !== "map") continue;
      try {
        const map = new MapRes(res.serial);
        let flowerc = 0;
        for (let i=map.v.length; i-->0; ) {
          if ((map.v[i] >= 0x05) && (map.v[i] <= 0x08)) {
            flowerc++;
          }
        }
        total += flowerc;
        report += `${flowerc} ${res.path}\n`;
      } catch (e) {
        report += `${res.path}: ${e.message}\n`;
      }
    }
    report += `${total} total`;
    this.dom.modalMessage(report);
  }
}

Custom.singleton = true;
