#include "AllegroGUI.hpp"

#include "../../Client.hpp"
#include "ImageLoader.hpp"

#include "../../LoaderSaver.hpp"

int main(int argc, char *argv[]) {

  try {
    std::cerr << "Starting..." << std::endl;
    AllegroGUI *gui = new AllegroGUI();
    std::cerr << "New game client" << std::endl;

    while(gui->running()) {

        //cerr << ".";
        while(gui->needUpdate())
        {
            gui->update();
        }

        gui->render();
        rest(0);
    }

    std::cerr << "Deleting game client..." << std::endl;
    delete gui;

    std::cerr << "Ending..." << std::endl;
  } catch (gcn::Exception &e) {
    std::cerr << e.getMessage() << std::endl;
    return 1;
  } catch (std::exception &e) {
    std::cerr << "Std exception: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "Unknown exception" << std::endl;
    return 1;
  }
  return 0;
} END_OF_MAIN()



// TODO - static functions instead? yes, do static
void AllegroGUI::handleSpeed(void *data) {
    ((AllegroGUI *)data)->speed_counter++;
} END_OF_FUNCTION(AllegroGUI::speedhandler)

void AllegroGUI::handleFPS(void *data) {
    ((AllegroGUI *)data)->fps    = ((AllegroGUI *)data)->frames;
    ((AllegroGUI *)data)->frames = 0;
} END_OF_FUNCTION(fpshandler)


AllegroGUI::AllegroGUI() {
  int resulotionX  = 800, resulotionY = 600;

  // Initializing allegro (and some sub elements)
  if(install_allegro(SYSTEM_AUTODETECT, &errno, atexit) != 0) {
    allegro_message("* Allegro could not be inited:\n  %s", allegro_error);
    exit(0);
  }

  set_color_depth(16);
  if(set_gfx_mode(GFX_AUTODETECT_WINDOWED, resulotionX, resulotionY, 0, 0) != 0) {
    allegro_message("* Graphics could not be inited:\n  %s", allegro_error);
    exit(0);
  }
  set_color_conversion(COLORCONV_TOTAL | COLORCONV_KEEP_TRANS);

  // No mouse, bitte!
  show_mouse(NULL);

  if(install_timer() != 0) {
    allegro_message("* Timers could not be inited:\n  %s", allegro_error);
    exit(0);
  } else if(install_keyboard() != 0) {
    allegro_message("* Keyboard could not be inited:\n  %s", allegro_error);
    exit(0);
  } else if(install_mouse() == -1) {
    allegro_message("* Mouse could not be inited:\n  %s", allegro_error);
    exit(0);
  }

  install_param_int_ex(AllegroGUI::handleSpeed, this, BPS_TO_TIMER(60));
  install_param_int_ex(AllegroGUI::handleFPS,   this, BPS_TO_TIMER(1));

  fps =  speed_counter =  frames = 0;

  // Locking variables and functions
  LOCK_VARIABLE(fps);
  LOCK_VARIABLE(speed_counter);
  LOCK_VARIABLE(frames);

  LOCK_FUNCTION(speedhandler);
  LOCK_FUNCTION(fpshandler);

  // Setting color depth
  std::cout << "Allegro inited..." << std::endl;

  // Create new client to work on
  client = new Client(this);

  // Create double buffer
  buffer = create_bitmap(SCREEN_W, SCREEN_H);

  if(!buffer) {
    allegro_message("Couldn't load / create some images");
    exit(1);
  }

  mr = new MapRender();
  mr->setMap(client->map);

  br = new BuildingRender();



  scrollSpeed = 5;

  try {

    menu_background = ImageLoader::getImage("img/menubg.pcx");
    gui_background  = ImageLoader::getImage("img/guibg.pcx");

    mouse_pointer   = ImageLoader::getImage("img/cursor.pcx");
    mouse_block     = ImageLoader::getImage("img/mouse_block.pcx");

  } catch(ImageLoaderException e) {
    allegro_message("Error: %s", e.what());
    exit(1);
  }

  // Set up guichan:


  imageLoader = new gcn::AllegroImageLoader();
  gcn::Image::setImageLoader(imageLoader);

  graphics = new gcn::AllegroGraphics();
  graphics->setTarget(buffer);

  input = new gcn::AllegroInput();

  top = new gcn::Container();
  // Set the dimension of the top container to match the screen.
  top->setDimension(gcn::Rectangle(0, 0, SCREEN_W, SCREEN_H));
  // Make it transparent
  top->setOpaque(false);

  gui = new gcn::Gui();
  // Set gui to use the AllegroGraphics object.
  gui->setGraphics(graphics);
  // Set gui to use the AllegroInput object
  gui->setInput(input);
  // Set the top container
  gui->setTop(top);

  gui->addGlobalKeyListener(this);
  // Load the image font.
  guiFont = new gcn::AllegroTrueTypeFont("DejaVuSans.ttf", 12);
  // The global font is static and must be set.
  gcn::Widget::setGlobalFont(guiFont);


  top->addMouseListener(this);

  console_show = false;
  usingTool    = false;

  economyWindow = new EconomyWindow();
  top->add(economyWindow, 100, 100);

  toolbar       = new Toolbar();
  top->add(toolbar, SCREEN_W - 10 - toolbar->getWidth(), 10);

  miniMap       = new MiniMap(client->map, &camera);
  top->add(miniMap, SCREEN_W - miniMap->getWidth() - 10,
      SCREEN_H - miniMap->getHeight() - 10);

}

AllegroGUI::~AllegroGUI(){

  delete client;

  delete mr;
  delete br;

  destroy_bitmap(menu_background);
  destroy_bitmap(gui_background);

  destroy_bitmap(mouse_pointer);
  destroy_bitmap(mouse_block);

  destroy_bitmap(buffer);

  // Guichan stuff
  delete input;
  delete graphics;
  delete imageLoader;

  delete toolbar;
  delete economyWindow;

  //delete guiFont;
  delete top;
  delete gui;

}

void AllegroGUI::mouseDragged (gcn::MouseEvent &e) {
  if(e.getSource() == top) {
    Point p = mr->toTileCoord(Point(e.getX(), e.getY()), camera);
    if(p.getX() < 0)
      p.setX(0);
    if(p.getY() < 0)
      p.setY(0);
    if(p.getX() >= (int)client->map->getWidth())
      p.setX((int)client->map->getWidth() - 1);
    if(p.getY() >= (int)client->map->getHeight())
      p.setY((int)client->map->getHeight() - 1);

    mouse_up_tile = p;
  }
}

void AllegroGUI::mousePressed (gcn::MouseEvent &e) {

  if(e.getSource() == top) {
    Point p = mr->toTileCoord(Point(e.getX(), e.getY()), camera);
    if(!client->map->outOfBounds(p)) {
      mouse_down_tile = mouse_up_tile = p;
      usingTool       = true;
    }
    
    if(e.getButton() == gcn::MouseEvent::RIGHT) {
      client->debug(mr->toTileCoord(Point(e.getX(), e.getY()), camera));
    }
  }
}
void AllegroGUI::mouseReleased (gcn::MouseEvent &e) {
  if(e.getSource() == top) {

    if(usingTool) {

      int tool = toolbar->getTool();

      if(tool == SIMULTY_CLIENT_TOOL_LAND) {
          // buy land
          client->buyLand(mouse_down_tile, mouse_up_tile);
      } else if(tool == SIMULTY_CLIENT_TOOL_ROAD) {
          // draw road
          client->buyRoad(mouse_down_tile, mouse_up_tile);
      } else if(tool == SIMULTY_CLIENT_TOOL_ZONE_RES ||
              tool == SIMULTY_CLIENT_TOOL_ZONE_COM ||
              tool == SIMULTY_CLIENT_TOOL_ZONE_IND) {
            // zone
          client->buyZone(mouse_down_tile, mouse_up_tile, tool);
      } else if(tool == SIMULTY_CLIENT_TOOL_BUILD_POLICE) {
        client->buyBuilding(mouse_down_tile, Building::TYPE_POLICE);
      } else if(tool == SIMULTY_CLIENT_TOOL_BUILD_FIRE) {
        client->buyBuilding(mouse_down_tile, Building::TYPE_FIRE);
      } else if(tool == SIMULTY_CLIENT_TOOL_BUILD_HOSPITAL) {
        client->buyBuilding(mouse_down_tile, Building::TYPE_HOSPITAL);
      } else if(tool == SIMULTY_CLIENT_TOOL_BULLDOZER) {
        client->bulldoze(mouse_down_tile, mouse_up_tile);
      } else if(tool == SIMULTY_CLIENT_TOOL_DEZONE) {
        client->deZone(mouse_down_tile, mouse_up_tile);
      }
      usingTool = false;
    }
  }
}


void AllegroGUI::keyPressed(gcn::KeyEvent &keyEvent) {

  if(keyEvent.getKey().getValue() == gcn::Key::UP) {
    camera.step(DIR_UP, 9, client->map->getWidth() * TILE_W - SCREEN_W,
        client->map->getHeight() * TILE_H - SCREEN_H);
  } else if(keyEvent.getKey().getValue() == gcn::Key::RIGHT) {
    camera.step(DIR_RIGHT, 9, client->map->getWidth() * TILE_W - SCREEN_W,
        client->map->getHeight() * TILE_H - SCREEN_H);
  } else if(keyEvent.getKey().getValue() == gcn::Key::DOWN) {
    camera.step(DIR_DOWN, 9, client->map->getWidth() * TILE_W - SCREEN_W,
        client->map->getHeight() * TILE_H - SCREEN_H);
  } else if(keyEvent.getKey().getValue() == gcn::Key::LEFT) {
    camera.step(DIR_LEFT, 9, client->map->getWidth() * TILE_W - SCREEN_W,
        client->map->getHeight() * TILE_H - SCREEN_H);
  }

  //std::cout << "KP: " << keyEvent.getKey().getValue() << std::endl;
}
void AllegroGUI::keyReleased(gcn::KeyEvent &keyEvent) {

  if(keyEvent.getKey().getValue() == gcn::Key::ESCAPE) {
    if(usingTool)
      usingTool = false;
    else
      client->state_running = false;
  } else if(keyEvent.getKey().getValue() == gcn::Key::F1) {
    console_show = !console_show;
  } else if(keyEvent.getKey().getValue() == 's') {
    //test = LoaderSaver::saveGame(client->map, NULL, NULL);
  } else if(keyEvent.getKey().getValue() == 'l') {
    //LoaderSaver::loadGame(test, client->map, NULL, NULL);
  }

  //std::cout << "KR: " << keyEvent.getKey().getValue() << std::endl;
}

void AllegroGUI::action(const gcn::ActionEvent &actionEvent) {

}


void AllegroGUI::render() {

  // Clear the double buffer:
  clear_bitmap(buffer);

  if(client->state_menu) {

    blit(menu_background, buffer, 0, 0, SCREEN_W - menu_background->w,
        SCREEN_H - menu_background->h, menu_background->w, menu_background->h);

    textprintf_ex(buffer, font, SCREEN_W - 200, SCREEN_H - 300, makecol(0, 0, 0), -1, "New local game");
    textprintf_ex(buffer, font, SCREEN_W - 200, SCREEN_H - 280, makecol(0, 0, 0), -1, "New network game");
    textprintf_ex(buffer, font, SCREEN_W - 200, SCREEN_H - 260, makecol(0, 0, 0), -1, "Join network game");

    textprintf_ex(buffer, font, SCREEN_W - 200, SCREEN_H - 200, makecol(0, 0, 0), -1, "Quit");


  } else if(client->state_game == SIMULTY_CLIENT_STATE_GAME_ON) {

    // Render map:
    mr->render(buffer, camera);
    // Render buildings:
    br->render(buffer, mr, camera, &client->bman);

    if(usingTool) {

      int cost = 0;
      Point c1 = mouse_down_tile;
      Point c3 = mouse_up_tile;

      int tool = toolbar->getTool();

      if(tool == SIMULTY_CLIENT_TOOL_ZONE_COM
          || tool == SIMULTY_CLIENT_TOOL_ZONE_RES
          || tool == SIMULTY_CLIENT_TOOL_ZONE_IND
          || tool == SIMULTY_CLIENT_TOOL_BULLDOZER
          || tool == SIMULTY_CLIENT_TOOL_LAND) {

        Point::fixOrder(c1, c3);
        //c3.translate(1, 1);

        Point c2 = Point(c3.getX(), c1.getY());
        Point c4 = Point(c1.getX(), c3.getY());

        c1 = mr->toScreenCoord(c1, camera); c2 = mr->toScreenCoord(c2, camera);
        c3 = mr->toScreenCoord(c3, camera); c4 = mr->toScreenCoord(c4, camera);

        int points[8] = { c1.getX() + TILE_W / 2, c1.getY(), // Upper left corner
                          c2.getX(), c2.getY() + TILE_H / 2,
                          c3.getX() + TILE_W / 2, c3.getY() + TILE_H, // Lower right corner
                          c4.getX() + TILE_W, c4.getY() + TILE_H / 2};

        int color = makecol(255, 255, 255);
        switch(tool) {
          case SIMULTY_CLIENT_TOOL_ZONE_COM:
            color = makecol(0, 0, 255);
            break;
          case SIMULTY_CLIENT_TOOL_ZONE_RES:
            color = makecol(0, 255, 0);
            break;
          case SIMULTY_CLIENT_TOOL_ZONE_IND:
            color = makecol(255, 0, 0);
            break;
          case SIMULTY_CLIENT_TOOL_LAND:
            color = makecol(255, 255, 255);
            break;
          case SIMULTY_CLIENT_TOOL_BULLDOZER:
            color = makecol(0, 0, 0);
            break;
        }
        set_trans_blender(255, 255, 255, 100);
        drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
        polygon(buffer, 4, points, color);
        solid_mode();
      } else if(tool == SIMULTY_CLIENT_TOOL_BUILD_POLICE) {

        // TODO
      }

      if(tool == SIMULTY_CLIENT_TOOL_ZONE_COM
          || tool == SIMULTY_CLIENT_TOOL_ZONE_RES
          || tool == SIMULTY_CLIENT_TOOL_ZONE_IND) {
        cost = client->map->buildZoneCost(client->getMyPlayer()->getSlot(),
            tool, mouse_down_tile, mouse_up_tile);
        //std::cout << "z: " << cost << " " << c1 << c3 << std::endl;
      } else if(tool == SIMULTY_CLIENT_TOOL_LAND) {
        cost = client->map->buyLandCost(client->getMyPlayer()->getSlot(),
            mouse_down_tile, mouse_up_tile);
        //std::cout << "l: " << cost << " " << c1 << c3 << std::endl;
      } else if(tool == SIMULTY_CLIENT_TOOL_BULLDOZER) {
        cost = client->map->bulldozeCost(client->getMyPlayer()->getSlot(),
            mouse_down_tile, mouse_up_tile);
      } else if(tool == SIMULTY_CLIENT_TOOL_DEZONE) {
        cost = client->map->deZoneCost(client->getMyPlayer()->getSlot(),
            mouse_down_tile, mouse_up_tile);
      }
      if(cost <= client->getMyPlayer()->getBudget()->getBalance())
        textprintf_ex(buffer, font, c3.getX(), c3.getY(),
            makecol(0, 0, 0), -1, "%i", cost);
      else
        textprintf_ex(buffer, font, c3.getX(), c3.getY(),
            makecol(255, 0, 0), -1, "%i", cost);

    }


    //blit(mouse_hint, buffer, 0, 0, (mouse_x / TILE_W) * TILE_W, (mouse_y / TILE_H) * TILE_H, mouse_hint->w, mouse_hint->h);

    Point realtile = mr->toTileCoord(mouse.getPosition(), camera);
    Point realscrn = mr->toScreenCoord(realtile, camera);

    // Redner mouse block
    masked_blit(mouse_block, buffer, 0, 0, realscrn.getX(), realscrn.getY(), mouse_block->w, mouse_block->h);

    // Render GUI:
    //masked_blit(gui_background, buffer, 0, 0, SCREEN_W - gui_background->w,
    //    SCREEN_H - gui_background->h, gui_background->w, gui_background->h);


    textprintf_ex(buffer, font, 20, SCREEN_H - 40, makecol(0, 0, 0), -1,
        "Money: %i", client->getMyPlayer()->getBudget()->getBalance());
    textprintf_ex(buffer, font, 20, SCREEN_H - 30, makecol(0, 0, 0), -1,
        "Time: %i %s %i", client->date.getYear(),
        client->date.getMonthAsString().c_str(), client->date.getDay());
    textprintf_ex(buffer, font, 20, SCREEN_H - 20, makecol(0, 0, 0), -1,
        "Tool: %i", toolbar->getTool());

    textprintf_ex(buffer, font, 200, SCREEN_H - 20, makecol(0, 0, 0), -1,
        "FPS: %i", fps);
    textprintf_ex(buffer, font, 200, SCREEN_H - 50, makecol(0, 0, 0), -1,
        "Camera: %i, %i (%i, %i)", camera.getX(), camera.getY(),
        mr->toTileCoord(camera).getX(), mr->toTileCoord(camera).getY());
    textprintf_ex(buffer, font, 200, SCREEN_H - 30, makecol(0, 0, 0), -1,
        "Mouse: %i, %i", realtile.getX(), realtile.getY());

    /* textprintf_ex(buffer, font, 400, SCREEN_H - 30, makecol(0, 0, 0), -1,
        "Thrive: %i", client->bman.getThrive(client->map,
        client->getMyPlayer()->getSlot(), realtile));
    textprintf_ex(buffer, font, 400, SCREEN_H - 15, makecol(0, 0, 0), -1,
        "Thrive level: %i", client->bman.getThriveLevel(client->map,
        client->getMyPlayer()->getSlot(), realtile)); */

    textprintf_ex(buffer, font, 600, SCREEN_H - 30, makecol(0, 0, 0), -1,
        "MD: %i, %i MU: %i, %i", mouse_down_tile.getX(),
        mouse_down_tile.getY(), mouse_up_tile.getX(), mouse_up_tile.getY());
    textprintf_ex(buffer, font, 600, SCREEN_H - 60, makecol(0, 0, 0), -1,
        "SB: %i", client->bman.getSpecialBuildingCount());

    // Draw console:
    if(console_show) {
        set_trans_blender(255, 255, 255, 100);
        drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
        rectfill(buffer, 0, 0, SCREEN_W, 100, makecol(50, 50, 50));
        for(int i = 1; i <= 5; i++)
        {
            if(console_data.size() - i >= 0 && console_data.size() - i < console_data.size())
                textprintf_ex(buffer, font, 10, 90 - 15*i, makecol(255, 255, 255), -1, "> %s", console_data[console_data.size() - i].c_str());
        }
        solid_mode();
    }

  }

  gui->draw();

  // Draw mouse pointer:
  masked_blit(mouse_pointer, buffer, 0, 0, mouse.getPosition().getX(), mouse.getPosition().getY(), 32, 32);

  // Render buffer to screen:
  blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

  // Increase number of frames:
  frames++;

}


void AllegroGUI::update() {

  gui->logic();
  client->update();

    mouse.update();

    if(client->state_menu) {
      if(mouse.getLeftButtonState() == STATE_PRESS) {
        client->state_menu = false; client->state_game = SIMULTY_CLIENT_STATE_GAME_START;
      }
    } else if(client->state_game == SIMULTY_CLIENT_STATE_GAME_ON) {

          economyWindow->update(client->getMyPlayer()->getBudget());

        if(mouse_y < 15)
          camera.step(DIR_UP, scrollSpeed, client->map->getWidth() * TILE_W
              - SCREEN_W, client->map->getHeight() * TILE_H - SCREEN_H);
        if(mouse_x > SCREEN_W - 15)
          camera.step(DIR_RIGHT, scrollSpeed, client->map->getWidth() * TILE_W
              - SCREEN_W, client->map->getHeight() * TILE_H - SCREEN_H);
        if(mouse_y > SCREEN_H - 15)
          camera.step(DIR_DOWN, scrollSpeed, client->map->getWidth() * TILE_W
              - SCREEN_W, client->map->getHeight() * TILE_H - SCREEN_H);
        if(mouse_x < 15)
          camera.step(DIR_LEFT, scrollSpeed, client->map->getWidth() * TILE_W
              - SCREEN_W, client->map->getHeight() * TILE_H - SCREEN_H);

        if(keypressed()) {


        }

    }

  speed_counter--;

}


void AllegroGUI::console_log(std::string s) {
    console_data.push_back(s);
}

bool AllegroGUI::needUpdate() {
    return speed_counter > 0;
}

bool AllegroGUI::running() {
  return client->state_running;
}
