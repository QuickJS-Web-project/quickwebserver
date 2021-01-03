import { readdir } from 'os';
import { SERVER_PATH } from '../env.js';

export function renderMenu() {
  const [list, err] = readdir(`${SERVER_PATH}/md`);
  if (err !== 0) throw new Error('Error reading menu!');
  const pagesOnly = list.filter((el) => el.includes('.md') && el[0] !== '_').sort();
  return pagesOnly
    .map((page) => {
      const withoutMd = page.replace('.md', '');
      let title = withoutMd.replace(/-/g, ' ').replace(/^(\d\d)\s(\w)/, '$2');
      title = title.charAt(0).toUpperCase() + title.slice(1);
      return `<div class="menu-elem"><a href="/page/${withoutMd}">${title}</a></div>`;
    })
    .join('');
}
