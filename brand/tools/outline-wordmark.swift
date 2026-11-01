// $Id$
// Elseport - convert the bundled wordmark font to portable vector outlines.
// SPDX-License-Identifier: GPL-3.0-only

import Foundation
import CoreText
import CoreGraphics

let fontURL = URL(fileURLWithPath: CommandLine.arguments[1])
let outputURL = URL(fileURLWithPath: CommandLine.arguments[2])
guard let provider = CGDataProvider(url: fontURL as CFURL), let graphicsFont = CGFont(provider) else {
    fatalError("Cannot read wordmark font")
}
let variation = [NSNumber(value: 2003265652): NSNumber(value: 650)]
let descriptor = CTFontDescriptorCreateWithAttributes([kCTFontVariationAttribute: variation] as CFDictionary)
let font = CTFontCreateWithGraphicsFont(graphicsFont, 100, nil, descriptor)
let text = NSAttributedString(string: "Elseport", attributes: [
    NSAttributedString.Key(kCTFontAttributeName as String): font,
    NSAttributedString.Key(kCTKernAttributeName as String): -3.3
])
let line = CTLineCreateWithAttributedString(text)
let width = CTLineGetTypographicBounds(line, nil, nil, nil)
var pathData = ""
func point(_ p: CGPoint, _ origin: CGPoint) -> String {
    return String(format: "%.3f %.3f", p.x + origin.x + 2, 105 - p.y - origin.y)
}
for run in CTLineGetGlyphRuns(line) as! [CTRun] {
    let count = CTRunGetGlyphCount(run)
    var glyphs = [CGGlyph](repeating: 0, count: count)
    var positions = [CGPoint](repeating: .zero, count: count)
    CTRunGetGlyphs(run, CFRange(location: 0, length: count), &glyphs)
    CTRunGetPositions(run, CFRange(location: 0, length: count), &positions)
    for index in 0..<count {
        guard let path = CTFontCreatePathForGlyph(font, glyphs[index], nil) else { continue }
        path.applyWithBlock { element in
            let e = element.pointee
            switch e.type {
            case .moveToPoint: pathData += "M" + point(e.points[0], positions[index])
            case .addLineToPoint: pathData += "L" + point(e.points[0], positions[index])
            case .addQuadCurveToPoint:
                pathData += "Q" + point(e.points[0], positions[index]) + " " + point(e.points[1], positions[index])
            case .addCurveToPoint:
                pathData += "C" + point(e.points[0], positions[index]) + " " + point(e.points[1], positions[index])
                    + " " + point(e.points[2], positions[index])
            case .closeSubpath: pathData += "Z"
            @unknown default: break
            }
        }
    }
}
let svg = """
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 \(Int(ceil(width + 4))) 130" fill="currentColor" role="img" aria-label="Elseport">
<title>Elseport wordmark</title>
<desc>Manrope 650, converted to outlines. Font: SIL Open Font License 1.1.</desc>
<path d="\(pathData)"/>
</svg>

"""
try svg.write(to: outputURL, atomically: true, encoding: .utf8)
print("Outlined Elseport wordmark, width \(width)")
