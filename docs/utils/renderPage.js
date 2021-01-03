import { SERVER_PATH } from '../env.js';
import { loadFile } from 'std';
import marked from './marked.js';
import { renderMenu } from './renderMenu.js';

export function renderMdPage(slug) {
  const fileName = `${slug}.md`;
  const content = loadFile(`${SERVER_PATH}/md/${fileName}`);
  if (!content) return null;
  const template = loadFile(`${SERVER_PATH}/_page.html`);
  return template.replace('{$ PAGE $}', marked(content)).replace('{$ MENU $}', renderMenu());
}
