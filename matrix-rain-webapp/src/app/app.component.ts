import html2canvas from 'html2canvas';

import { NgFor, NgStyle } from '@angular/common';
import { Component, ElementRef, ViewChild, OnInit } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { RouterOutlet } from '@angular/router';
import { NbButtonModule, NbCardModule, NbCheckboxModule, NbInputModule, NbLayoutModule, NbSelectModule } from '@nebular/theme';

@Component({
  selector: 'app-root',
  standalone: true,
  imports: [
    RouterOutlet,
    FormsModule,
    NgStyle,
    NgFor,
    NbLayoutModule,
    NbButtonModule,
    NbCardModule,
    NbInputModule,
    NbSelectModule,
    NbCheckboxModule,
  ],
  templateUrl: './app.component.html',
  styleUrl: './app.component.scss'
})
export class AppComponent implements OnInit {
  @ViewChild("outputSet") outputElement!: ElementRef;
  title = 'matrix-rain-webapp';
  fontFamily = 'monospace';
  fontSize = 40;
  charWidth = 40;
  charHeight = 40;
  lineHeight = 40;
  fontUnit = 'px';
  charWidthUnit = 'px';
  charHeightUnit = 'px';
  lineHeightUnit = 'px';
  chars = 'ABCDEFG';
  characterArray: string[] = [];
  bold = false;
  italic = false;

  style: {[key: string]: any} = {};
  charStyle: {[key: string]: any} = {};

  ngOnInit(): void {
    this.updateFont();
  }

  async updateFont() {
    this.style = {};
    this.charStyle = {};
    this.style[`font-size.${this.fontUnit}`] = this.fontSize;
    this.style[`height.${this.charHeightUnit}`] = this.charHeight;
    this.style[`line-height.${this.lineHeightUnit}`] = this.lineHeight;
    if (this.fontFamily && this.fontFamily.trim() !== '') {
      this.style['--font-family'] = this.fontFamily;
    }
    this.charStyle[`width.${this.charWidthUnit}`] = this.charWidth;
    this.charStyle[`height.${this.charHeightUnit}`] = this.charHeight;
    if (this.bold) {
      this.charStyle[`font-weight`] = "bold"
    }
    if(this.italic) {
      this.charStyle[`font-style`] = "italic"
    }
    await this.updateSet();
  }

  async takeAPicture() {
    const canvas = await html2canvas(this.outputElement.nativeElement);
    const imageData = canvas.toDataURL("image/png");
    
    const link = document.createElement("a");
    link.setAttribute("download", "texture.png");
    link.setAttribute("href", imageData);
    link.click();
  }
  
  async updateSet() {
    // Remove all new line, return characters, and spaces
    const textToSplit = this.chars.replace(/[\r\n\s\t]/g, '');
    this.characterArray = textToSplit.split('');

    //await updatePreview(e);
  }
}

